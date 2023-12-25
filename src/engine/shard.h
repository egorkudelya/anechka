#pragma once

#include "common.h"
#include "umap.h"
#include "xxh64_hasher.h"
#include "doc_trace.h"
#include <memory>

namespace std
{
    template<typename K, typename V>
    struct hash<std::pair<K, V>>
    {
        inline size_t operator()(const std::pair<K, V>& v) const
        {
            size_t seed = 0;
            utils::hash_combine(seed, v.first);
            utils::hash_combine(seed, v.second);
            return seed;
        }
    };
}

namespace core
{
    using TokenRecord = core::USet<std::pair<std::string_view, size_t>>;
    using TokenRecordPtr = std::shared_ptr<TokenRecord>;
    using ConstTokenRecordPtr = std::shared_ptr<const TokenRecord>;

    class Shard
    {
    public:
        Shard(float maxLoadFactor, size_t expectedTokenCount);
        void insert(std::string&& token, const std::string& doc, DocStat&& docStat, size_t pos);
        ConstTokenRecordPtr search(const std::string& token, bool& exists) const;
        void erase(const std::string& token);
        bool exists(const std::string& token) const;
        float loadFactor() const;
        bool isExpandable() const;
        size_t tokenCount() const;
        size_t docCount() const;
        size_t tokenCountPerDoc(const std::string& path) const;
        Json serialize() const;

    private:
        /**
        * word -> [document_path, position]
        */
        SUMap<std::string, TokenRecordPtr, Xxh64Hasher> m_record;
        DocTrace m_docTrace;
        const float m_maxLoadFactor;
    };

    using ShardPtr = std::shared_ptr<Shard>;
}