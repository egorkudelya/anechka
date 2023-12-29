#pragma once

#include "shard.h"
#include "../thread_pool/pool/thread_pool.h"

namespace core
{
    namespace detail
    {
        template<typename Begin, typename End>
        auto tokenizeRange(Begin begin, End end, bool toLowercase)
        {
            auto rangeToStr = [](auto begin, auto end, bool lower) {
                std::string token{begin, end};
                if (lower)
                {
                    utils::toLower(token);
                }
                return token;
            };

            std::vector<std::pair<std::string, size_t>> res;
            for (auto p = begin;; ++p)
            {
                auto q = p;
                p = std::find_if_not(p, end, [](char ch) {
                    return utils::isLetter(ch) || ch == '\'';
                });

                if (p != q)
                {
                    const auto pos = p - begin;
                    res.emplace_back(rangeToStr(q, p, toLowercase), pos);
                }

                if (p == end)
                {
                    return res;
                }
            }
        }
    }

    namespace CacheType
    {
        enum Type : int8_t
        {
            ContextSearch = 1,
            QuerySearch = 2,
        };
        const Type All[] = {ContextSearch, QuerySearch};
    }

    namespace cache
    {
        // universal cache buffer for each CacheType::Type in json format
        using CacheEntry = std::string;
        using CacheTypeEntry = SUMap<std::string, CacheEntry>;
        using CacheTypeEntryPtr = std::shared_ptr<CacheTypeEntry>;

        class Cache
        {
        public:
            Cache(size_t reserve, float maxLF);
            void insert(CacheType::Type cacheType, const CacheTypeEntryPtr& entryPtr);
            void cache(const std::string& key, const std::string& json, CacheType::Type cacheType);
            void invalidate();
            CacheEntry search(const std::string& key, CacheType::Type cacheType, bool& found) const;

        private:
            SUMap<CacheType::Type, CacheTypeEntryPtr> m_cache;
            std::atomic<bool> m_clear{true};
            float m_maxLF{0.75};
        };
        using CachePtr = std::shared_ptr<Cache>;
    }

    namespace tfidf
    {
        inline bool docRankCompare(const std::pair<std::string_view, float>& first,
                                   const std::pair<std::string_view, float>& second)
        {
            return first.second > second.second;
        }

        using RankedDocs = std::vector<std::pair<std::string, float>>;
    }

    struct SearchEngineParams
    {
        size_t size;
        size_t docs;
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
        void insert(std::string&& token, const std::string& doc, DocStat&& docStat, size_t pos);
        void erase(const std::string& token);
        ConstTokenRecordPtr search(std::string token, bool& found) const;
        tfidf::RankedDocs searchQuery(std::string query);
        void cache(const std::string& key, const std::string& json, CacheType::Type cacheType);
        cache::CacheEntry searchCache(const std::string& key, CacheType::Type cacheType, bool& found) const;
        void invalidateCache();
        size_t tokenCount() const;
        size_t docCount() const;
        Json serialize() const;
        float loadFactor() const;
        bool isExpandable() const;
        bool dump(const std::string& strPath) const;
        bool restore(const std::string& strPath, bool& isStale);

    private:
        void initCache(size_t reserve);

    private:
        SearchEngineParams m_params;
        SemanticParams m_semantics;
        ThreadPoolPtr m_pool;
        ShardPtr m_storage;
        cache::CachePtr m_cache;
    };

    using SearchEnginePtr = std::unique_ptr<SearchEngine>;
}