#pragma once

#include "shard.h"
#include "../thread_pool/pool/thread_pool.h"

namespace core
{
    // TODO cache responses for search queries with context
    struct SearchEngineParams
    {
        size_t size;
        size_t threads;
        float maxLF;
        bool toLowercase;
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
        void insertToken(std::string&& token, const std::string& doc, size_t pos);
        void erase(const std::string& token);
        ConstTokenRecordPtr search(const std::string& token, bool& found) const;
        size_t tokenCount() const;
        Json serialize() const;
        float loadFactor() const;
        bool isExpandable() const;
        bool dump(const std::string& strPath) const;
        bool restore(const std::string& strPath, bool& isStale);

    private:
        SemanticParams m_semantics;
        ThreadPoolPtr m_pool;
        ShardPtr m_storage;
    };

    using SearchEnginePtr = std::unique_ptr<SearchEngine>;
}