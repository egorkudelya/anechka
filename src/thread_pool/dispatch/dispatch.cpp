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
        std::unique_lock<std::shared_mutex> lock(m_mapMtx);
        m_map[threadId] = queuePtr;
    }

    Task QueueDispatch::extractTaskByThreadId(size_t threadId)
    {
        Map::iterator iter;
        {
            std::shared_lock<std::shared_mutex> lock(m_mapMtx);
            iter = m_map.find(threadId);
            if (iter == m_map.end())
            {
                return {};
            }
        }
        Task task;
        iter->second->pop(task);
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
        auto nodeHandle = m_set.extract(m_set.begin());
        return nodeHandle.value();
    }

    void QueueDispatch::insertQueue(const QueuePtr& queueBlockPtr)
    {
        std::unique_lock<std::shared_mutex> lock(m_setMtx);
        m_set.insert(queueBlockPtr);
    }

    void QueueDispatch::clear()
    {
        std::unique_lock<std::shared_mutex> lock(m_mapMtx);
        for (auto&& bucket: m_map)
        {
            bucket.second->signalAbort();
        }
    }
}