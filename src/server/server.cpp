#include "server.h"
#include "../mmap/mmap.h"
#include "timer.h"
#include <fstream>
#include <sstream>

namespace anechka
{
    static const auto delims = {'.', ',', '!', '?'};

    static std::string escape(const std::string& src)
    {
        std::string res;
        res.reserve(src.size());
        for (char ch: src)
        {
            switch (ch)
            {
                case '\'': res += "\\'"; break;
                case '\"': res += "\\\""; break;
                case '\?': res += "\\?"; break;
                case '\\': res += "\\\\"; break;
                case '\a': res += "\\a"; break;
                case '\b': res += "\\b"; break;
                case '\f': res += "\\f"; break;
                case '\n': res += "\\n"; break;
                case '\r': res += "\\r"; break;
                case '\0': res += "\\0"; break;
                case '\t': res += "\\t"; break;
                case '\v': res += "\\v"; break;
                default: res += ch;
            }
        }
        return res;
    }

    static std::string contextualize(const std::unique_ptr<const MMapASCII>& mmap, size_t idx)
    {
        if (mmap->size() <= idx)
        {
            return {};
        }

        ssize_t leftCursor = idx - 10;
        ssize_t rightCursor = idx + 10;
        while (leftCursor > 0 && rightCursor < mmap->size())
        {
            bool l = std::find(delims.begin(), delims.end(), (*mmap)[leftCursor]) == delims.end();
            bool r = std::find(delims.begin(), delims.end(), (*mmap)[rightCursor]) == delims.end();
            if (l)
            {
                leftCursor--;
            }
            if (r)
            {
                rightCursor++;
            }
            if (!l && !r)
            {
                leftCursor++;
                break;
            }
        }
        if (rightCursor - leftCursor == 0 || leftCursor < 0 || rightCursor >= mmap->size())
        {
            leftCursor = 0;
            rightCursor = 0;
        }
        auto begin = mmap->begin() + leftCursor;
        auto end = mmap->begin() + rightCursor;

        std::string body{begin, end};
        utils::removeControlChars(body);

        return "..." + std::move(body) + "...";
    }

    Anechka::Anechka(const std::string& configPath)
    {
        config(configPath);
    }

    Anechka::~Anechka()
    {
        if (m_persistent)
        {
            m_searchEngine->dump(m_dumpPath);
        }
    }

    void Anechka::config(const std::string& configPath)
    {
        std::ifstream file(configPath);
        if (file.fail())
        {
            throw std::invalid_argument("Invalid path to config file");
        }

        const Json config = Json::parse(file);
        const size_t hardwareThreads = std::thread::hardware_concurrency();

        m_port = !config["port"].is_null() ? config["port"].get<int>() : 4444;
        m_threadCount = !config["serv_threads"].is_null() ? config["serv_threads"].get<size_t>() : hardwareThreads;
        m_persistent = config["is_persistent"].is_null() || config["is_persistent"].get<bool>();
        bool toLowercase = config["to_lowercase"].is_null() || config["to_lowercase"].get<bool>();
        bool restoring = config["is_restoring_on_start"].is_null() || config["is_restoring_on_start"].get<bool>();
        float maxLF = !config["max_load_factor"].is_null() ? config["max_load_factor"].get<float>() : 0.75;
        size_t size = !config["est_unique_tokens"].is_null() ? config["est_unique_tokens"].get<size_t>() : 3e5;
        size_t threads = !config["se_threads"].is_null() ? config["se_threads"].get<size_t>() : hardwareThreads;
        float cacheSize = !config["cache_size"].is_null() ? config["cache_size"].get<float>() : 0.25;

        const core::SearchEngineParams engineParams{size, threads, maxLF, toLowercase, cacheSize};
        m_searchEngine = std::make_unique<core::SearchEngine>(engineParams);

        if (restoring)
        {
            bool stale;
            if (!m_searchEngine->restore(m_dumpPath, stale))
            {
                std::cerr << "Failed to load index from dump. Make sure it exists in the anechka/dump/ directory\n";
                return;
            }
            if (stale)
            {
                std::cerr << "Loading stale index from dump. Some referenced files have been modified "
                             "since the last back-up\n";
            }
        }
    }

    net::ResponsePtr Anechka::RequestTxtFileIndexing(const net::RequestPtr& requestPtr)
    {
        utils::Timer timer{};
        auto pathRequestPtr = utils::downcast<net::Path::Path>(requestPtr);
        auto responsePtr = std::make_shared<net::InsertResponse::InsertResponse>();

        std::string path = pathRequestPtr->getPath();
        utils::removeControlChars(path);

        bool ok = m_searchEngine->indexTxtFile(std::move(path));
        if (!m_searchEngine->isExpandable())
        {
            std::stringstream warning;
            warning << "The cluster is overloaded, the current load factor: " << m_searchEngine->loadFactor()
                    << ". This means more collisions during further insertions and slower response times. "
                       "Please adjust est_unique_tokens and/or max_load_factor accordingly";
            responsePtr->getEnginestatus() = warning.str();
        }
        else
        {
            responsePtr->getEnginestatus() = "nominal";
        }

        responsePtr->getOk() = ok;
        responsePtr->getIndexsize() = m_searchEngine->tokenCount();
        responsePtr->getTook() = std::to_string(timer.getInterval());

        return responsePtr;
    }

    net::ResponsePtr Anechka::RequestRecursiveDirIndexing(const net::RequestPtr& requestPtr)
    {
        utils::Timer timer{};
        auto pathRequestPtr = utils::downcast<net::Path::Path>(requestPtr);
        auto responsePtr = std::make_shared<net::InsertResponse::InsertResponse>();

        const std::string& path = pathRequestPtr->getPath();
        bool ok = m_searchEngine->indexDir(path);
        if (!m_searchEngine->isExpandable())
        {
            std::stringstream warning;
            warning << "The cluster is overloaded, the current load factor: " << m_searchEngine->loadFactor()
                    << ". This means more collisions during further insertions and slower response times. "
                       "Please adjust est_unique_tokens and/or max_load_factor accordingly";
            responsePtr->getEnginestatus() = warning.str();
        }
        else
        {
            responsePtr->getEnginestatus() = "nominal";
        }

        responsePtr->getOk() = ok;
        responsePtr->getIndexsize() = m_searchEngine->tokenCount();
        responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";

        return responsePtr;
    }

    net::ResponsePtr Anechka::RequestTokenSearch(const net::RequestPtr& requestPtr)
    {
        utils::Timer timer{};
        auto tokenRequestPtr = utils::downcast<net::BasicToken::BasicToken>(requestPtr);
        auto responsePtr = std::make_shared<net::SearchResponse::SearchResponse>();

        const std::string& token = tokenRequestPtr->getToken();

        bool found = false;
        auto tokenPtr = m_searchEngine->search(token, found);

        if (found)
        {
            responsePtr->getJson() = tokenPtr->serialize().dump(2);
        }
        else
        {
            responsePtr->getJson() = R"({"status": "not found"})";
        }
        responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";

        return responsePtr;
    }

    net::ResponsePtr Anechka::RequestTokenDeletion(const net::RequestPtr& requestPtr)
    {
        utils::Timer timer{};
        auto tokenRequestPtr = utils::downcast<net::BasicToken::BasicToken>(requestPtr);
        auto responsePtr = std::make_shared<net::EraseResponse::EraseResponse>();

        bool found = true;
        const std::string& token = tokenRequestPtr->getToken();

        m_searchEngine->erase(token);
        m_searchEngine->search(token, found);
        responsePtr->getOk() = !found;
        responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";

        return responsePtr;
    }

    net::ResponsePtr Anechka::RequestTokenSearchWithContext(const net::RequestPtr& requestPtr)
    {
        utils::Timer timer{};
        auto tokenRequestPtr = utils::downcast<net::BasicToken::BasicToken>(requestPtr);
        auto responsePtr = std::make_shared<net::ContextResponse::ContextResponse>();

        const std::string& token = tokenRequestPtr->getToken();

        bool foundCache = false;
        auto entry = m_searchEngine->searchCache(core::CacheType::ContextSearch, token, foundCache);
        if (foundCache)
        {
            responsePtr->getResult() = entry;
            responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";
            return responsePtr;
        }

        bool foundPrimary = false;
        auto tokenPtr = m_searchEngine->search(token, foundPrimary);
        if (!foundPrimary)
        {
            return responsePtr;
        }

        std::vector<std::string>& index = responsePtr->getResult();

        std::unordered_multimap<std::string, std::string> contexts;
        for (const auto& pair: tokenPtr->serialize())
        {
            size_t i = pair.at(1);
            const std::string& path = pair.at(0);
            std::unique_ptr<const MMapASCII> mmap;
            try
            {
                mmap = std::make_unique<const MMapASCII>(path);
            }
            catch (const std::exception& err)
            {
                return responsePtr;
            }

            contexts.emplace(path, escape(contextualize(mmap, i)));
        }

        for (auto iter = contexts.begin(); iter != contexts.end();)
        {
            Json jcontexts;
            auto end = contexts.equal_range(iter->first).second;

            std::string key = iter->first;
            for (; iter != end; ++iter)
            {
                jcontexts.push_back(iter->second);
            }

            Json final;
            final["path"] = std::move(key);
            final["contexts"] = std::move(jcontexts.dump(2));

            index.push_back(final.dump(2));
        }
        responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";
        m_searchEngine->cacheTokenContext(token, index);

        return responsePtr;
    }
}