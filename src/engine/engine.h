#pragma once

#include "shard.h"
#include "../thread_pool/pool/thread_pool.h"

namespace core
{
    namespace CacheType
    {
        enum Type
        {
            ContextSearch = 1,
        };
        static const Type All[] = {ContextSearch};
    }

    // universal cache buffer for each CacheType::Type
    using CacheEntry = std::vector<std::string>;

    using Cache = SUMap<std::pair<CacheType::Type, std::string>, CacheEntry>;
    using CachePtr = std::shared_ptr<Cache>;

    struct SearchEngineParams
    {
        size_t size;
        size_t threads;
        float maxLF;
        bool toLowercase;
        float cacheSize;
    };

    class SearchEngine
    {
        struct SemanticParams
        {
            bool lcaseTokens{false};
        };

    public:
        explicit SearchEngine(const SearchEngineParams& params);
        bool indexDir(const std::string& strPath);
        bool indexTxtFile(std::string&& strPath);
        void insert(std::string&& token, const std::string& doc, size_t pos);
        void erase(const std::string& token);
        ConstTokenRecordPtr search(const std::string& token, bool& found) const;
        void cacheTokenContext(const std::string& token, const std::vector<std::string>& contexts);
        CacheEntry searchCache(CacheType::Type cacheType, const std::string& token, bool& found) const;
        void invalidateCache(const std::string& token);
        size_t tokenCount() const;
        size_t docCount() const;
        Json serialize() const;
        float loadFactor() const;
        bool isExpandable() const;
        bool dump(const std::string& strPath) const;
        bool restore(const std::string& strPath, bool& isStale);

    private:
        SearchEngineParams m_params;
        SemanticParams m_semantics;
        ThreadPoolPtr m_pool;
        ShardPtr m_storage;
        CachePtr m_cache;
    };

    using SearchEnginePtr = std::unique_ptr<SearchEngine>;
}