#pragma once

#include "common.h"
#include <functional>
#include <list>
#include <random>
#include <memory>
#include <shared_mutex>

/**
 * SUMap stands for Static (Safe) Unordered Map
 */

namespace core
{
    template<typename Key, typename Value, typename Hash = std::hash<Key>>
    class SUMap
    {
        using BucketValue = std::pair<Key, Value>;

        class BucketType
        {
            using BucketData = std::list<BucketValue>;
            using BucketIterator = typename BucketData::iterator;
            using ConstBucketIterator = typename BucketData::const_iterator;

            BucketIterator findUnsafe(const Key& key)
            {
                return std::find_if(m_data.begin(), m_data.end(), [&](const BucketValue& item) {
                    return item.first == key;
                });
            }

            ConstBucketIterator findUnsafe(const Key& key) const
            {
                return std::find_if(m_data.cbegin(), m_data.cend(), [&](const BucketValue& item) {
                    return item.first == key;
                });
            }

        public:
            bool exists(const Key& key) const
            {
                std::shared_lock<std::shared_mutex> lock(m_mtx);
                return findUnsafe(key) != m_data.end();
            }

            Value get(const Key& key, const Value& defaultValue) const
            {
                std::shared_lock<std::shared_mutex> lock(m_mtx);
                ConstBucketIterator record = findUnsafe(key);
                if (record == m_data.end())
                {
                    return defaultValue;
                }
                return record->second;
            }

            void insert(const Key& key, const Value& value, bool& isNew)
            {
                std::unique_lock<std::shared_mutex> lock(m_mtx);
                const BucketIterator record = findUnsafe(key);
                if (record == m_data.end())
                {
                    m_data.push_back(std::make_pair(key, value));
                    isNew = true;
                }
                else
                {
                    record->second = value;
                    isNew = false;
                }
            }

            template<typename PrimaryCallBack, typename SecondaryCallBack>
            void _insert(const Key& key, bool& isNew, PrimaryCallBack&& primary, SecondaryCallBack&& secondary)
            {
                std::unique_lock<std::shared_mutex> lock(m_mtx);
                const BucketIterator record = findUnsafe(key);
                if (record == m_data.end())
                {
                    m_data.push_back(std::make_pair(key, primary()));
                    isNew = true;
                }
                else
                {
                    auto value = *record;
                    lock.unlock();
                    secondary(std::move(value));
                    isNew = false;
                }
            }

            void emplace(BucketValue&& bucket, bool& isNew)
            {
                std::unique_lock<std::shared_mutex> lock(m_mtx);
                BucketIterator record = findUnsafe(bucket.first);
                if (record == m_data.end())
                {
                    m_data.emplace_back(std::move(bucket));
                    isNew = true;
                }
                else
                {
                    *record = std::move(bucket);
                    isNew = false;
                }
            }

            void erase(const Key& key, bool& isErased)
            {
                std::unique_lock<std::shared_mutex> lock(m_mtx);
                BucketIterator record = findUnsafe(key);
                if (record == m_data.end())
                {
                    isErased = false;
                    return;
                }
                m_data.erase(record);
                isErased = true;
            }

            Key eraseRandom(const Key& defaultKey)
            {
                std::unique_lock<std::shared_mutex> lock(m_mtx);
                if (m_data.empty())
                {
                    return defaultKey;
                }
                BucketIterator target = m_data.begin();
                m_data.erase(target);
                return target->first;
            }

            Json serialize() const
            {
                Json bucket;
                std::unique_lock<std::shared_mutex> lock(m_mtx);
                for (const BucketValue& data: m_data)
                {
                    Json pair;
                    pair.emplace(data.first, utils::serializeType(std::move(data.second)));
                    bucket.push_back(std::move(pair));
                }
                return bucket;
            }

        private:
            BucketData m_data;
            mutable std::shared_mutex m_mtx;
        };

        const BucketType& getBucket(const Key& key) const
        {
            const size_t bucketIndex = m_hasher(key) % m_buckets.size();
            return *m_buckets[bucketIndex];
        }

        BucketType& getBucket(const Key& key)
        {
            return const_cast<BucketType&>(const_cast<const SUMap*>(this)->getBucket(key));
        }

        BucketType& getBucket(size_t i, bool tag)
        {
            return *m_buckets[i];
        }

        auto initBuckets(size_t reserveSize) const
        {
            if (reserveSize == 0)
            {
                reserveSize = 1;
            }
            std::vector<std::unique_ptr<BucketType>> buckets;
            buckets.reserve(reserveSize);
            for (size_t i = 0; i < reserveSize; i++)
            {
                buckets.emplace_back(std::make_unique<BucketType>());
            }
            return buckets;
        }

    public:
        using KeyType = Key;
        using ValueType = Value;

        explicit SUMap(size_t reserveSize = 419)
            : m_hasher(Hash{})
            , m_size(0)
            , m_buckets(std::move(initBuckets(reserveSize)))
            , m_bucketCount(m_buckets.size())
        {
        }

        bool exists(const Key& key) const
        {
            return getBucket(key).exists(key);
        }

        Value get(const Key& key, const Value& defaultValue = Value()) const
        {
            return getBucket(key).get(key, defaultValue);
        }

        void insert(const Key& key, const Value& value)
        {
            bool isNew;
            getBucket(key).insert(key, value, isNew);
            if (isNew)
            {
                m_size++;
            }
        }

        template<typename PrimaryCallBack, typename SecondaryCallBack>
        void _insert(const Key& key, PrimaryCallBack&& primary, SecondaryCallBack&& secondary)
        {
            bool isNew;
            getBucket(key)
                ._insert(key, isNew, std::forward<PrimaryCallBack>(primary), std::forward<SecondaryCallBack>(secondary));
            if (isNew)
            {
                m_size++;
            }
        }

        void emplace(BucketValue&& bucket)
        {
            bool isNew;
            getBucket(bucket.first).emplace(std::move(bucket), isNew);
            if (isNew)
            {
                m_size++;
            }
        }

        void erase(const Key& key)
        {
            bool isErased;
            getBucket(key).erase(key, isErased);
            if (isErased)
            {
                m_size--;
            }
        }

        Key eraseRandom(const Key& defaultKey = Key())
        {
            /**
            * since m_buckets is const, we have a guarantee that the number of buckets
            * will remain constant during the lifetime of SUmap.
            */

            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(0, m_buckets.size() - 1);

            Key erased = defaultKey;
            for (size_t i = 0; i < m_buckets.size(); i++)
            {
                erased = getBucket(dist(rng), true).eraseRandom(defaultKey);
                if (erased != defaultKey)
                {
                    break;
                }
            }
            return erased;
        }

        size_t size() const noexcept
        {
            /**
            * The result may be a bit outdated for calling threads when called during a mutable
            * operation taking place in some other thread, but it is acceptable in this case
            */
            return m_size;
        }

        size_t bucketCount() const noexcept
        {
            return m_bucketCount;
        }

        float loadFactor() const noexcept
        {
            float lf = m_size / (float)m_bucketCount;
            if (lf > 1.0f)
            {
                return 1.0f;
            }
            return lf;
        }

        Json serialize() const
        {
            Json map;
            for (const auto& bucket: m_buckets)
            {
                auto serializedList = bucket->serialize();
                for (auto&& obj: serializedList)
                {
                    if (!obj.is_null())
                    {
                        map.update(std::move(obj));
                    }
                }
            }
            return map;
        }

    private:
        const std::vector<std::unique_ptr<BucketType>> m_buckets;
        std::atomic<size_t> m_size;
        const size_t m_bucketCount;
        const Hash m_hasher;
    };

}// namespace core