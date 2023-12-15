#pragma once

#include <atomic>
#include <condition_variable>
#include <queue>
#include <shared_mutex>

namespace core
{
    template<typename T>
    class SafeQueue
    {

    public:
        explicit SafeQueue(size_t id = 0)
            : m_id(id){};

        void push(T&& task)
        {
            std::unique_lock lock(m_mtx);
            m_tasks.emplace(std::move(task));
            m_size++;
            m_cv.notify_one();
        }

        bool pop(T& value)
        {
            {
                std::unique_lock lock(m_mtx);
                m_cv.wait(lock, [&] {
                    return !m_tasks.empty() || m_abort;
                });

                if (m_abort)
                {
                    return false;
                }

                value = std::move(m_tasks.front());
                m_tasks.pop();
                m_size--;
            }
            return true;
        }

        void signalAbort()
        {
            m_abort = true;
            m_cv.notify_all();
        }

        size_t getId() const
        {
            return m_id;
        }

        size_t getCurrentSize() const
        {
            std::shared_lock lock(m_mtx);
            return m_size;
        }

        bool isEmpty() const
        {
            std::shared_lock lock(m_mtx);
            return m_size == 0;
        }

    private:
        size_t m_id;
        size_t m_size{0};
        std::atomic<bool> m_abort{false};
        std::condition_variable_any m_cv;
        std::queue<T> m_tasks;
        mutable std::shared_mutex m_mtx;
    };

    template<typename T>
    using SafeQueuePtr = std::shared_ptr<SafeQueue<T>>;

    struct QueueCompare
    {
        template<typename T>
        bool operator()(const SafeQueuePtr<T>& lhs, const SafeQueuePtr<T>& rhs) const
        {
            if (lhs->getCurrentSize() != rhs->getCurrentSize())
            {
                return lhs->getCurrentSize() < rhs->getCurrentSize();
            }
            return lhs->getId() < rhs->getId();
        }
    };
}