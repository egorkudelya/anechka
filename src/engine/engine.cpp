#include "engine.h"
#include "../mmap/mmap.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace core
{
    static auto tokenize(const std::unique_ptr<const MMapASCII>& mmap, bool toLowercase=false)
    {
        return detail::tokenizeRange(mmap->begin(), mmap->end(), toLowercase);
    }

    static auto tokenize(const std::string& query, bool toLowercase=false)
    {
        return detail::tokenizeRange(query.begin(), query.end(), toLowercase);
    }

    cache::Cache::Cache(size_t reserve, float maxLF)
        : m_cache(reserve)
        , m_maxLF(maxLF)
    {
    }

    void cache::Cache::cache(const std::string& key, const std::string& json, CacheType::Type cacheType)
    {
        m_clear = false;
        const cache::CacheTypeEntryPtr& typeEntry = m_cache.get(cacheType);
        if (typeEntry->loadFactor() > m_maxLF)
        {
            typeEntry->eraseRandom();
        }

        typeEntry->insert(key, json);
    }

    void cache::Cache::invalidate()
    {
        if (m_clear)
        {
            return;
        }
        for (const auto& cacheType: CacheType::All)
        {
            const cache::CacheTypeEntryPtr& typeEntry = m_cache.get(cacheType);
            if (typeEntry)
            {
                typeEntry->drop();
            }
        }
        m_clear = true;
    }

    cache::CacheEntry cache::Cache::search(const std::string& key, CacheType::Type cacheType, bool& found) const
    {
        cache::CacheTypeEntryPtr typeEntry = m_cache.get(cacheType);
        if (!typeEntry)
        {
            found = false;
            return {};
        }
        cache::CacheEntry entry = typeEntry->get(key);
        found = !entry.empty();
        return entry;
    }

    void cache::Cache::insert(CacheType::Type cacheType, const cache::CacheTypeEntryPtr& entryPtr)
    {
        m_cache.insert(cacheType, entryPtr);
    }

    SearchEngine::SearchEngine(const SearchEngineParams& params)
    {
        m_params = params;
        const size_t queues = params.threads / 3 > 0 ? params.threads / 3 : 1;

        m_storage = std::make_shared<Shard>(params.maxLF, params.size, params.docs);
        m_cache = std::make_shared<cache::Cache>((sizeof(CacheType::All) / 8) * 2, params.maxLF);
        m_pool = std::make_unique<ThreadPool>(queues, params.threads, true);
        m_semantics = SemanticParams{params.toLowercase};

        initCache(params.size * params.cacheSize);
    }

    void SearchEngine::initCache(size_t reserve)
    {
        for (const auto& cacheType: CacheType::All)
        {
            m_cache->insert(cacheType, std::make_shared<SUMap<std::string, cache::CacheEntry>>(reserve));
        }
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

        auto tokens = tokenize(mmap, m_semantics.lcaseTokens);
//        printf("%s%s%s%zu\n", "Indexing ", strPath.c_str(), "; tokens: ", tokens.size());

        for (auto&& token: tokens)
        {
            insert(std::move(token.first), strPath, {tokens.size()}, token.second);
        }
        return true;
    }

    void SearchEngine::insert(std::string&& token, const std::string& doc, DocStat&& docStat, size_t pos)
    {
        invalidateCache();
        m_storage->insert(std::move(token), doc, std::move(docStat), pos);
    }

    ConstTokenRecordPtr SearchEngine::search(std::string token, bool& found) const
    {
        if (m_params.toLowercase)
        {
            utils::toLower(token);
        }
        return m_storage->search(token, found);
    }

    static void rankDocs(SUMap<std::string, float>& ranks, const std::string& path, float tfIdf)
    {
        ranks.mutate(path, [tfIdf](const auto& it, bool iterValid, bool& shouldErase) {
            shouldErase = false;
            if (iterValid)
            {
                it->second += tfIdf;
                return 0.0f;
            }
            return tfIdf;
        });
    }

    tfidf::RankedDocs SearchEngine::searchQuery(std::string query)
    {
        if (m_params.toLowercase)
        {
            utils::toLower(query);
        }

        SUMap<std::string, float> ranks(m_storage->docCount() * 10e-2);
        {
            std::vector<WaitableFuture> futures;
            for (auto&&[token, _]: tokenize(query))
            {
                futures.push_back(m_pool->submitTask(
                    [this, &ranks, tok = std::move(token)] {
                        bool found;
                        const auto recordPtr = search(tok, found);
                        if (!found)
                        {
                            return;
                        }

                        std::unordered_map<std::string_view, size_t> hist;
                        for (auto&[path, _pos]: recordPtr->iterate())
                        {
                            hist[path]++;
                        }

                        const float idf = log10f((float)m_storage->docCount() / hist.size());
                        for (auto&&[pathView, freq]: hist)
                        {
                            std::string path{pathView};
                            const float tf = (float)freq / m_storage->tokenCountForDoc(path);
                            rankDocs(ranks, path, tf * idf);
                        }
                    },
                    true));
            }
        }

        auto snapshot = ranks.snapshotDense();
        const size_t topX = snapshot.size() < 20 ? snapshot.size() : 20;

        auto threshold = snapshot.begin() + topX;
        std::partial_sort(snapshot.begin(), threshold, snapshot.end(), [](auto&& first, auto&& second) {
            return tfidf::docRankCompare(first, second);
        });

        return tfidf::RankedDocs {
            std::make_move_iterator(snapshot.begin()),
            std::make_move_iterator(threshold)
        };
    }

    void SearchEngine::erase(const std::string& token)
    {
        invalidateCache();
        return m_storage->erase(token);
    }

    void SearchEngine::cache(const std::string& key, const std::string& json, CacheType::Type cacheType)
    {
        m_cache->cache(key, json, cacheType);
    }

    cache::CacheEntry SearchEngine::searchCache(const std::string& key, CacheType::Type cacheType, bool& found) const
    {
        return m_cache->search(key, cacheType, found);
    }

    void SearchEngine::invalidateCache()
    {
        m_cache->invalidate();
    }

    size_t SearchEngine::tokenCount() const
    {
        return m_storage->tokenCount();
    }

    size_t SearchEngine::docCount() const
    {
        return m_storage->docCount();
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

        Json data = {{"timestamp", ns.count()}};
        data.update(m_storage->serialize());
        if (path.extension() == ".json")
        {
            std::ofstream f(path);
            if (f.fail())
            {
                return false;
            }
            f << data;
            return true;
        }
        if (path.extension() == ".bson")
        {
            std::ofstream f(path, std::ios::binary);
            if (f.fail())
            {
                return false;
            }
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

        Json backup;
        if (path.extension() == ".json")
        {
            std::ifstream f(path);
            if (f.fail())
            {
                return false;
            }
            backup = Json::parse(f);
        }
        else if (path.extension() == ".bson")
        {
            std::ifstream f(path, std::ios::binary);
            if (f.fail())
            {
                return false;
            }
            std::vector<uint8_t> bjdata(std::istreambuf_iterator<char>(f), {});
            backup = Json::from_bjdata(bjdata.begin(), bjdata.end());
        }
        else
        {
            return false;
        }

        size_t dumpTs = backup.at("timestamp").get<size_t>();
        const Json& dump = backup.at("dump");
        const Json& docTrace = backup.at("docTrace");

        for (auto&& [token, info]: dump.items())
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

                DocStat docStat = docTrace.at(docPath).get<DocStat>();
                m_storage->insert(std::string{token}, docPath, std::move(docStat), it.at(1));
            }
        }
        return true;
    }
}