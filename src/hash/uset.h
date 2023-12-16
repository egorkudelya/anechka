#pragma once

#include "json_spec.h"
#include <memory>
#include <functional>
#include <unordered_set>
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
                static_assert(std::is_same_v<Value, URValue> == true);

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

            Json serialize() const
            {
                Json bucket;
                std::unique_lock<std::shared_mutex> lock(m_mtx);
                for (auto&& data: m_data)
                {
                    bucket.push_back(utils::serializeType(std::move(data)));
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

    public:
        using ValueType = Value;
        using HashType = Hash;

        explicit USet(size_t reserveSize = 9)
            : m_hasher(Hash{})
            , m_size(0)
        {
            if (reserveSize == 0)
            {
                reserveSize = 1;
            }
            m_buckets.reserve(reserveSize);
            for (size_t i = 0; i < reserveSize; i++)
            {
                m_buckets.emplace_back(std::make_unique<BucketType>());
            }
        }

        template<typename URValue>
        void addIfNotPresent(URValue&& value)
        {
            static_assert(std::is_same_v<Value, URValue>);

            bool isPresentAlready;
            getBucket(value).addIfNotPresent(std::forward<URValue>(value), isPresentAlready);
            if (!isPresentAlready)
            {
                m_size++;
            }
        }

        bool contains(const Value& value) const
        {
            return getBucket(value).contains(value);
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

    private:
        std::vector<std::unique_ptr<BucketType>> m_buckets;
        std::atomic<size_t> m_size;
        Hash m_hasher;
    };
}