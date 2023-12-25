#pragma once

#include "json_spec.h"
#include <memory>
#include <functional>
#include <unordered_set>
#include <list>
#include <shared_mutex>
#include <mutex>
#include <atomic>

namespace core
{
    template<typename Value, typename Hash = std::hash<Value>>
    class USet
    {
        class BucketType
        {
            using BucketValue = Value;
            using BucketData = std::unordered_set<BucketValue>;
            using BucketIterator = typename BucketData::iterator;

            BucketIterator findUnsafe(const BucketValue& value)
            {
                return m_data.find(value);
            }

        public:
            template<typename URValue>
            void addIfNotPresent(URValue&& value, bool& isPresentAlready)
            {
                static_assert(std::is_same_v<Value, std::decay_t<URValue>>);

                std::unique_lock<std::shared_mutex> lock(m_mtx);
                BucketIterator record = findUnsafe(value);
                if (record == m_data.end())
                {
                    isPresentAlready = false;
                    m_data.insert(std::forward<URValue>(value));
                    return;
                }
                isPresentAlready = true;
            }

            void erase(const Value& value, bool& isErased)
            {
                std::unique_lock<std::shared_mutex> lock(m_mtx);
                BucketIterator record = findUnsafe(value);
                if (record == m_data.end())
                {
                    isErased = false;
                    return;
                }
                m_data.erase(record);
                isErased = true;
            }

            bool contains(const Value& value) const
            {
                std::shared_lock<std::shared_mutex> lock(m_mtx);
                BucketIterator record = findUnsafe(value);
                if (record == m_data.end())
                {
                    return false;
                }
                return true;
            }

            std::list<Value> snapshot() const
            {
                std::list<Value> values;
                std::shared_lock<std::shared_mutex> lock(m_mtx);
                return std::list<Value>(m_data.begin(), m_data.end());
            }

            Json serialize() const
            {
                Json bucket;
                std::shared_lock<std::shared_mutex> lock(m_mtx);
                for (const auto& data: m_data)
                {
                    bucket.push_back(utils::serializeType(data));
                }
                return bucket;
            }

        private:
            BucketData m_data;
            mutable std::shared_mutex m_mtx;
        };

        BucketType& getBucket(const Value& value)
        {
            const size_t bucketIndex = m_hasher(value) % m_buckets.size();
            return *m_buckets[bucketIndex];
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
        using ValueType = Value;
        using HashType = Hash;

        explicit USet(size_t reserveSize = 9)
            : m_hasher(Hash{})
            , m_size(0)
            , m_buckets(initBuckets(reserveSize))
        {
        }

        template<typename URValue>
        void addIfNotPresent(URValue&& value)
        {
            static_assert(std::is_same_v<Value, std::decay_t<URValue>>);

            bool isPresentAlready;
            getBucket(value).addIfNotPresent(std::forward<URValue>(value), isPresentAlready);
            if (!isPresentAlready)
            {
                m_size++;
            }
        }

        bool erase(const Value& value)
        {
            bool isErased;
            getBucket(value).erase(value, isErased);
            if (isErased)
            {
                m_size--;
            }
            return isErased;
        }

        bool contains(const Value& value) const
        {
            return getBucket(value).contains(value);
        }

        std::list<Value> snapshot() const
        {
            std::list<Value> values;
            for (const auto& bucket: m_buckets)
            {
                values.splice(values.end(), bucket->snapshot());
            }
            return values;
        }

        Json serialize() const
        {
            Json set;
            for (const auto& bucket: m_buckets)
            {
                auto serializedList = bucket->serialize();
                for (auto&& obj: serializedList)
                {
                    if (!obj.is_null())
                    {
                        set.push_back(std::move(obj));
                    }
                }
            }
            return set;
        }

        size_t size() const noexcept
        {
            /**
            * The result may be a bit outdated for calling threads when called during a mutable
            * operation taking place in some other thread, but it is acceptable in this case
            */
            return m_size;
        }

        float loadFactor() const noexcept
        {
            float lf = m_size / (float)m_buckets.size();
            if (lf > 1.0f)
            {
                return 1.0f;
            }
            return lf;
        }

    private:
        const std::vector<std::unique_ptr<BucketType>> m_buckets;
        std::atomic<size_t> m_size;
        Hash m_hasher;
    };
}