#pragma once

#include "safe_queue.h"
#include "../dispatch/dispatch.h"
#include "../primitives/waitable_future.h"
#include <condition_variable>
#include <thread>
#include <vector>

namespace core
{
    class ThreadPool
    {
    public:
        ThreadPool(size_t numberOfQueues = 3, size_t threadsPerQueue = 2, bool isGraceful = true);
        ~ThreadPool();

        ThreadPool() = delete;
        ThreadPool(const ThreadPool& other) = delete;
        ThreadPool& operator=(ThreadPool&& other) = delete;
        ThreadPool(ThreadPool&& other) noexcept = delete;
        ThreadPool& operator=(const ThreadPool& other) noexcept = delete;

        template<typename Invocable>
        WaitableFuture submitTask(Invocable&& invocable, bool isWaiting = false)
        {
            if (m_shutDown || !m_isInitialized)
            {
                return {};
            }
            size_t thisId = std::hash<std::thread::id>{}(std::this_thread::get_id());
            Task task(CallBack(std::forward<Invocable>(invocable), thisId));
            WaitableFuture future{std::move(task.getFuture()), isWaiting};
            m_jobCount++;
            m_primaryDispatch.pushTaskToLeastBusy(std::move(task));
            return future;
        }

    private:
        void waitForAll();
        void process();

    private:
        size_t m_threadCount;
        QueueDispatch m_primaryDispatch;
        std::vector<std::thread> m_threads;
        bool m_shutDown{false};
        bool m_isInitialized{false};
        bool m_isGraceful{false};
        std::mutex m_waitMtx;
        std::mutex m_poolMtx;
        std::condition_variable m_waitCv;
        std::atomic<int64_t> m_jobCount{0};
        std::atomic<int64_t> m_runningJobCount{0};
    };

    using ThreadPoolPtr = std::unique_ptr<ThreadPool>;
}