#pragma once

#include "../primitives/callback.h"
#include "../primitives/task.h"
#include "umap.h"
#include "safe_queue.h"
#include <set>
#include <shared_mutex>

namespace core
{
    using Task = PackagedTask<void>;
    using Queue = SafeQueue<Task>;
    using QueuePtr = std::shared_ptr<Queue>;

    class QueueDispatch
    {
        using Set = std::set<QueuePtr, QueueCompare>;
        using Map = SUMap<size_t, QueuePtr>;

    public:
        QueueDispatch() = default;
        size_t size() const;
        void insertQueue(const QueuePtr& queueBlockPtr);
        void assignThreadIdToQueue(size_t threadId, const QueuePtr& queuePtr);
        void pushTaskToLeastBusy(Task&& callBack);
        Task extractTaskByThreadId(size_t threadId);
        void clear();

    private:
        QueuePtr extractLeastBusy();

    private:
        std::condition_variable_any m_cv;
        mutable std::shared_mutex m_setMtx;
        Set m_set;
        Map m_map;
    };
}