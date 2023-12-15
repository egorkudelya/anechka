#include "thread_pool.h"

namespace core
{
    ThreadPool::ThreadPool(size_t numberOfQueues, size_t threadsPerQueue, bool isGraceful)
        : m_isGraceful(isGraceful)
    {
        for (size_t i = 0; i < numberOfQueues; i++)
        {
            auto queue = std::make_shared<Queue>(i);
            m_primaryDispatch.insertQueue(queue);
            for (size_t j = 0; j < threadsPerQueue; j++)
            {
                std::thread thread = std::thread([this, queue] {
                    size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
                    m_primaryDispatch.assignThreadIdToQueue(id, queue);
                    process();
                });
                m_threads.emplace_back(std::move(thread));
            }
        }
        m_threadCount = numberOfQueues * threadsPerQueue;
        m_isInitialized = true;
    }

    void ThreadPool::process()
    {
        size_t thisId = std::hash<std::thread::id>{}(std::this_thread::get_id());
        while (true)
        {
            if (m_shutDown)
            {
                return;
            }
            Task task;
            if (m_isInitialized)
            {
                task = m_primaryDispatch.extractTaskByThreadId(thisId);
            }
            if (task.valid())
            {
                m_jobCount--;
                m_runningJobCount++;
                task();
                m_runningJobCount--;
                m_waitCv.notify_one();
            }
        }
    }

    void ThreadPool::waitForAll()
    {
        std::unique_lock lock(m_waitMtx);
        m_waitCv.wait(lock, [&] {
            return m_jobCount == 0 && m_runningJobCount == 0;
        });
    }

    ThreadPool::~ThreadPool()
    {
        std::unique_lock lock(m_poolMtx);
        if (m_isGraceful)
        {
            waitForAll();
        }
        m_shutDown = true;
        m_primaryDispatch.clear();
        for (auto&& thread: m_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
        m_isInitialized = false;
    }
}