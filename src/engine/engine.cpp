#include "engine.h"
#include "../mmap/mmap.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace core
{
    static const auto delims = {';', ' ', '(', ')', '\"', '.', '?', '!', ',', ':', '-', '\n', '\t', '\'', '/', '>'};

    static auto tokenize(const std::unique_ptr<const MMapASCII>& mmap, bool toLowercase=false)
    {
        std::vector<std::pair<std::string, size_t>> res;
        for (auto p = mmap->begin();; ++p)
        {
            auto q = p;
            p = std::find_first_of(p, mmap->end(), delims.begin(), delims.end());

            if (p == mmap->end())
            {
                return res;
            }
            const auto pos = p - mmap->begin();
            std::string token{q, p};
            if (toLowercase)
            {
                utils::toLower(token);
            }

            res.emplace_back(token, pos);
        }
    }

    SearchEngine::SearchEngine(const SearchEngineParams& params)
    {
        m_params = params;
        const size_t queues = params.threads / 3 > 0 ? params.threads / 3 : 1;

        m_storage = std::make_shared<Shard>(params.maxLF, params.size);
        m_cache = std::make_shared<Cache>(params.size * params.cacheSize);
        m_pool = std::make_unique<ThreadPool>(queues, params.threads, true);
        m_semantics = SemanticParams{params.toLowercase};
    }

    bool SearchEngine::indexDir(const std::string& strPath)
    {
        const std::filesystem::path dirPath = std::filesystem::u8path(strPath);
        if (!std::filesystem::exists(dirPath))
        {
            return false;
        }

        std::vector<WaitableFuture> futures;
        for (const auto& dirEntry: std::filesystem::recursive_directory_iterator(strPath))
        {
            std::filesystem::path path = dirEntry.path();
            if (path.extension() == ".txt")
            {
                futures.push_back(m_pool->submitTask(
                    [this, path = std::move(path)] {
                        indexTxtFile(std::move(path.relative_path()));
                    },
                    true));
            }
        }
        return true;
    }

    bool SearchEngine::indexTxtFile(std::string&& strPath)
    {
        const std::filesystem::path path = std::filesystem::u8path(strPath);
        if (path.extension() != ".txt")
        {
            return false;
        }
        if (!std::filesystem::exists(path))
        {
            return false;
        }

        std::unique_ptr<const MMapASCII> mmap;
        try
        {
            mmap = std::make_unique<const MMapASCII>(path);
        }
        catch (const std::exception& err)
        {
            return false;
        }

        for (auto&& token: tokenize(mmap, m_semantics.lcaseTokens))
        {
            insert(std::move(token.first), strPath, token.second);
        }
        return true;
    }

    void SearchEngine::insert(std::string&& token, const std::string& doc, size_t pos)
    {
        invalidateCache(token);
        m_storage->insert(std::move(token), doc, pos);
    }

    ConstTokenRecordPtr SearchEngine::search(const std::string& token, bool& found) const
    {
        return m_storage->search(token, found);
    }

    void SearchEngine::erase(const std::string& token)
    {
        invalidateCache(token);
        return m_storage->erase(token);
    }

    void SearchEngine::cacheTokenContext(const std::string& token, const std::vector<std::string>& contexts)
    {
        if (m_cache->loadFactor() > m_params.maxLF)
        {
            m_cache->eraseRandom();
        }
        m_cache->insert(std::make_pair(CacheType::ContextSearch, token), contexts);
    }

    CacheEntry SearchEngine::searchCache(CacheType::Type cacheType, const std::string& token, bool& found) const
    {
        CacheEntry entry = m_cache->get(std::make_pair(cacheType, token));
        found = !entry.empty();
        return entry;
    }

    void SearchEngine::invalidateCache(const std::string& token)
    {
        for (const auto& cacheType: CacheType::All)
        {
            m_cache->erase(std::make_pair(cacheType, token));
        }
    }

    size_t SearchEngine::tokenCount() const
    {
        return m_storage->size();
    }

    Json SearchEngine::serialize() const
    {
        return m_storage->serialize();
    }

    float SearchEngine::loadFactor() const
    {
        return m_storage->loadFactor();
    }

    bool SearchEngine::isExpandable() const
    {
        return m_storage->isExpandable();
    }

    bool SearchEngine::dump(const std::string& strPath) const
    {
        // TODO compression
        const std::filesystem::path path = std::filesystem::u8path(strPath);
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());

        if (path.extension() == ".json")
        {
            std::ofstream f(path);
            if (f.fail())
            {
                return false;
            }
            f << Json{{"timestamp", ns.count()}, {"dump", m_storage->serialize()}};
            return true;
        }
        if (path.extension() == ".bson")
        {
            std::ofstream f(path, std::ios::binary);
            if (f.fail())
            {
                return false;
            }
            Json data = {{"timestamp", ns.count()}, {"dump", m_storage->serialize()}};
            std::vector<uint8_t> bjdata = Json::to_bjdata(data);
            f.write(reinterpret_cast<const char*>(bjdata.data()), bjdata.size());
            return true;
        }
        return false;
    }

    bool SearchEngine::restore(const std::string& strPath, bool& isStale)
    {
        isStale = false;
        const std::filesystem::path path = std::filesystem::u8path(strPath);

        Json dump;
        if (path.extension() == ".json")
        {
            std::ifstream f(path);
            if (f.fail())
            {
                return false;
            }
            dump = Json::parse(f);
        }
        else if (path.extension() == ".bson")
        {
            std::ifstream f(path, std::ios::binary);
            if (f.fail())
            {
                return false;
            }
            std::vector<uint8_t> bjdata(std::istreambuf_iterator<char>(f), {});
            dump = Json::from_bjdata(bjdata.begin(), bjdata.end());
        }
        else
        {
            return false;
        }

        const Json& index = dump.at("dump");
        size_t dumpTs = dump.at("timestamp").get<size_t>();
        for (auto&& [token, info]: index.items())
        {
            for (auto& it: info)
            {
                const std::string& docPath = it.at(0).get<std::string>();
                auto ftime = std::filesystem::last_write_time(docPath);
                if (dumpTs < ftime.time_since_epoch().count() && !isStale)
                {
                    /**
                    * File has been mutated since the last back-up
                    */
                    isStale = true;
                }
                m_storage->insert(std::string{token}, docPath, it.at(1));
            }
        }
        return true;
    }
}