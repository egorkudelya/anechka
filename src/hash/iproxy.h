#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <shared_mutex>


namespace core
{
    template<typename Container, template<typename> typename Lock = std::unique_lock>
    class IterableProxy
    {
        using ValueType = typename Container::ValueType;
        using BucketType = typename Container::BucketType;
        using BucketTypePtr = std::unique_ptr<BucketType>;
        using BufferType = std::vector<BucketTypePtr>;

    public:
        class Iterator
        {
            using BucketIterator = typename BucketType::Iterator;
            void advanceTillValid()
            {
                while (m_bucketId < m_buffer.size() && m_crawl == m_buffer[m_bucketId]->end())
                {
                    const BucketTypePtr& currBucket = m_buffer[m_bucketId];
                    std::ptrdiff_t distance = std::distance(currBucket->begin(), m_crawl);
                    if (currBucket->size() > 0 && distance < currBucket->size() - 1)
                    {
                        m_crawl++;
                    }
                    else if (m_bucketId < m_buffer.size() - 1)
                    {
                        m_bucketId++;
                        m_crawl = m_buffer[m_bucketId]->begin();
                    }
                    else
                    {
                        break;
                    }
                }
            }

        public:
            explicit Iterator(const BufferType& buffer, BucketIterator iter, size_t bucketId = 0)
                : m_buffer(buffer)
                , m_crawl(iter)
                , m_bucketId(bucketId)
            {
                advanceTillValid();
            }

            const Iterator& operator++()
            {
                if (m_crawl != m_buffer[m_bucketId]->end())
                {
                    m_crawl++;
                }
                else if (m_bucketId < m_buffer.size())
                {
                    m_bucketId++;
                    m_crawl = m_buffer[m_bucketId]->begin();
                }
                advanceTillValid();
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator retval = *this;
                ++(*this);
                return retval;
            }

            bool operator!=(const Iterator& other) const
            {
                if (m_bucketId != other.m_bucketId && m_crawl != m_buffer[m_bucketId]->end())
                {
                    return true;
                }
                return m_crawl != other.m_crawl;
            }

            bool operator==(const Iterator& other) const
            {
                return !operator!=(other);
            }

            const auto& operator*() const
            {
                return *m_crawl;
            }

        private:
            size_t m_bucketId;
            BucketIterator m_crawl;
            const BufferType& m_buffer;
        };

        explicit IterableProxy(const BufferType& buffer, std::shared_mutex& mtx)
            : m_lock(mtx)
            , m_buffer(buffer)
        {
        }

        Iterator begin() const
        {
            const BucketTypePtr& bucket = *m_buffer.begin();
            bucket->begin();
            return Iterator(m_buffer, bucket->begin(), 0);
        }

        Iterator end() const
        {
            const BucketTypePtr& bucket = m_buffer[m_buffer.size() - 1];
            return Iterator(m_buffer, bucket->end(), m_buffer.size());
        }

    private:
        Lock<std::shared_mutex> m_lock;
        const BufferType& m_buffer;
    };
}