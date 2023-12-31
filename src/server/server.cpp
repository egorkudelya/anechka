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

        int64_t leftCursor = idx - 10;
        int64_t rightCursor = idx + 10;
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

        return "..." + utils::lrtrim(body) + "...";
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

        m_port = utils::getJsonProperty<int>(config, "port", 4444);
        m_threadCount = utils::getJsonProperty<size_t>(config, "serv_threads", hardwareThreads);
        m_persistent = utils::getJsonProperty<bool>(config, "is_persistent", false);
        bool toLowercase = utils::getJsonProperty<bool>(config, "to_lowercase", false);
        bool restoring = utils::getJsonProperty<bool>(config, "is_restoring_on_start", false);
        float maxLF = utils::getJsonProperty<float>(config, "max_load_factor", 0.75);
        size_t size = utils::getJsonProperty<size_t>(config, "est_distinct_tokens", 3e5);
        size_t docs = utils::getJsonProperty<size_t>(config, "est_docs", 10e3);
        size_t threads = utils::getJsonProperty<size_t>(config, "se_threads", hardwareThreads);
        float cacheSize = utils::getJsonProperty<float>(config, "cache_size", 0.25);

        const core::SearchEngineParams engineParams{size, docs, threads, maxLF, toLowercase, cacheSize};
        m_searchEngine = std::make_unique<core::SearchEngine>(engineParams);

        if (restoring)
        {
            bool stale;
            if (!m_searchEngine->restore(m_dumpPath, stale))
            {
                std::cerr << "Failed to load index from dump. Make sure it exists in the dump/ directory\n";
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
                       "Please adjust est_distinct_tokens and/or max_load_factor accordingly";
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
                       "Please adjust est_distinct_tokens and/or max_load_factor accordingly";
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
            std::vector<std::string> final;
            size_t threshold = 50;
            for (auto&&[path, pos]: tokenPtr->iterate())
            {
                if (threshold == 0)
                {
                    break;
                }
                Json docEntry{{"path", path}, {"pos", pos}};
                final.push_back(docEntry.dump(2));
                threshold--;
            }
            responsePtr->getResponse() = final;
        }
        else
        {
            responsePtr->getResponse() = {};
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
        auto responsePtr = std::make_shared<net::ContextSearchResponse::ContextSearchResponse>();

        const std::string& token = tokenRequestPtr->getToken();

        bool foundCache = false;
        auto entry = m_searchEngine->searchCache(token, core::CacheType::ContextSearch, foundCache);
        if (foundCache)
        {
            responsePtr->getResponses() = Json::parse(entry).get<std::vector<std::string>>();
            responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";
            return responsePtr;
        }

        bool foundPrimary = false;
        auto tokenPtr = m_searchEngine->search(token, foundPrimary);
        if (!foundPrimary)
        {
            return responsePtr;
        }

        std::vector<std::string>& index = responsePtr->getResponses();
        std::unordered_multimap<std::string, std::string> contexts;
        for (auto&&[path, i]: tokenPtr->iterate())
        {
            std::unique_ptr<const MMapASCII> mmap;
            try
            {
                mmap = std::make_unique<const MMapASCII>(std::string{path});
            }
            catch (const std::exception& err)
            {
                continue;
            }

            contexts.emplace(path, escape(contextualize(mmap, i)));
        }

        size_t threshold = 50;
        for (auto iter = contexts.begin(); iter != contexts.end();)
        {
            if (threshold == 0)
            {
                break;
            }
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
            threshold--;
        }
        responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";
        m_searchEngine->cache(token, Json(index).dump(), core::CacheType::ContextSearch);

        return responsePtr;
    }

    net::ResponsePtr Anechka::RequestQuerySearch(const net::RequestPtr& requestPtr)
    {
        utils::Timer timer{};
        auto queryRequestPtr = utils::downcast<net::SearchQueryRequest::SearchQueryRequest>(requestPtr);
        auto responsePtr = std::make_shared<net::SearchQueryResponse::SearchQueryResponse>();

        const std::string& query = queryRequestPtr->getQuery();

        bool foundCache = false;
        auto cachedRank = m_searchEngine->searchCache(query, core::CacheType::QuerySearch, foundCache);
        if (foundCache)
        {
            responsePtr->getRankeddocs() = Json::parse(cachedRank).get<std::vector<std::string>>();
            responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";

            return responsePtr;
        }

        core::tfidf::RankedDocs ranked = m_searchEngine->searchQuery(queryRequestPtr->getQuery());

        std::vector<std::string> final;
        final.reserve(ranked.size());
        for (const auto& docRes: ranked)
        {
            Json docEntry{{"path", docRes.first}, {"rank", docRes.second}};
            final.push_back(docEntry.dump(2));
        }

        responsePtr->getRankeddocs() = final;
        responsePtr->getTook() = std::to_string(timer.getInterval()) + "ms";

        m_searchEngine->cache(query, {Json(final).dump()}, core::CacheType::QuerySearch);

        return responsePtr;
    }
}