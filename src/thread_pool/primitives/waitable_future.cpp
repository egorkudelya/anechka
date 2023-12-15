#include "waitable_future.h"

namespace core
{
    WaitableFuture::WaitableFuture()
    {
        std::future<void> defaultFuture;
        m_future = std::make_unique<Model<std::future<void>>>(std::move(defaultFuture));
        m_isWaiting = false;
    }

    bool WaitableFuture::valid() const
    {
        return m_future->valid();
    }

    WaitableFuture::~WaitableFuture()
    {
        if (m_isWaiting && m_future && m_future->valid())
        {
            m_future->wait();
        }
    }
}