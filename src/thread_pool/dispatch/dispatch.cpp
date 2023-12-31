#include "dispatch.h"

namespace core
{
    size_t QueueDispatch::size() const
    {
        std::shared_lock<std::shared_mutex> lock(m_setMtx);
        return m_set.size();
    }

    void QueueDispatch::assignThreadIdToQueue(size_t threadId, const QueuePtr& queuePtr)
    {
        m_map.insert(threadId, queuePtr);
    }

    Task QueueDispatch::extractTaskByThreadId(size_t threadId)
    {
        QueuePtr queue;
        {
            queue = m_map.get(threadId, nullptr);
            if (!queue)
            {
                return {};
            }
        }
        Task task;
        queue->pop(task);
        return task;
    }

    void QueueDispatch::pushTaskToLeastBusy(Task&& task)
    {
        QueuePtr queue = extractLeastBusy();
        queue->push(std::move(task));
        insertQueue(queue);
    }

    QueuePtr QueueDispatch::extractLeastBusy()
    {
        std::unique_lock<std::shared_mutex> lock(m_setMtx);
        m_cv.wait(lock, [this] {
            return !m_set.empty();
        });

        auto nodeHandle = m_set.extract(m_set.begin());
        return nodeHandle.value();
    }

    void QueueDispatch::insertQueue(const QueuePtr& queueBlockPtr)
    {
        std::unique_lock<std::shared_mutex> lock(m_setMtx);
        m_set.insert(queueBlockPtr);
        m_cv.notify_one();
    }

    void QueueDispatch::clear()
    {
        for (auto&& bucket: m_map.iterate())
        {
            bucket.second->signalAbort();
        }
    }
}