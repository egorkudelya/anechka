#pragma once

#include "iproxy.h"
#include "json_spec.h"
#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>

namespace core
{
    template<typename Value, typename Hash = std::hash<Value>>
    class USet
    {
        class BucketType
        {
            friend class IterableProxy<const USet<Value, Hash>>;

            using BucketValue = Value;
            using BucketData = std::unordered_set<BucketValue>;

            auto begin()
            {
                /**
                * Not intended for direct use
                */
                return m_data.begin();
            }

            auto end()
            {
                /**
                * Not intended for direct use
                */
                return m_data.end();
            }

        public:
            using Iterator = typename BucketData::iterator;
            using ConstIterator = typename BucketData::const_iterator;

            template<typename URValue>
            void addIfNotPresent(URValue&& value, bool& isPresentAlready)
            {
                static_assert(std::is_constructible_v<Value, std::decay_t<URValue>>);

                std::unique_lock<std::shared_mutex> lock(m_mtx);
                Iterator record = findUnsafe(value);
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
                Iterator record = findUnsafe(value);
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
                const ConstIterator record = findUnsafe(value);
                if (record == m_data.end())
                {
                    return false;
                }
                return true;
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

            size_t size() const
            {
                std::shared_lock<std::shared_mutex> lock(m_mtx);
                return m_data.size();
            }

        private:
            Iterator findUnsafe(const BucketValue& value)
            {
                return m_data.find(value);
            }

            ConstIterator findUnsafe(const BucketValue& value) const
            {
                return std::as_const(m_data).find(value);
            }

            BucketData m_data;
            mutable std::shared_mutex m_mtx;
        };

        BucketType& getBucket(const Value& value)
        {
            const size_t bucketIndex = m_hasher(value) % m_buckets.size();
            return *m_buckets[bucketIndex];
        }

        const BucketType& getBucket(const Value& value) const
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
        using BucketType = BucketType;
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
            static_assert(std::is_constructible_v<Value, std::decay_t<URValue>>);

            std::shared_lock<std::shared_mutex> lock(m_globalMtx);
            bool isPresentAlready;
            getBucket(value).addIfNotPresent(std::forward<URValue>(value), isPresentAlready);
            if (!isPresentAlready)
            {
                m_size++;
            }
        }

        bool erase(const Value& value)
        {
            std::shared_lock<std::shared_mutex> lock(m_globalMtx);
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

        inline auto iterate() const
        {
            return IterableProxy<const USet<ValueType, HashType>, std::unique_lock>(m_buckets, m_globalMtx);
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
        mutable std::shared_mutex m_globalMtx;
        const std::vector<std::unique_ptr<BucketType>> m_buckets;
        std::atomic<size_t> m_size;
        Hash m_hasher;
    };
}