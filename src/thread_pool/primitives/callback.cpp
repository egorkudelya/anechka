#include "callback.h"

namespace core
{
    CallBack::CallBack(const CallBack& other)
        : m_self(other.m_self->copy())
        , m_senderId(other.m_senderId)
    {
    }

    CallBack& CallBack::operator=(const CallBack& other)
    {
        CallBack tmp(other);
        *this = std::move(tmp);
        return *this;
    }

    void CallBack::operator()() const
    {
        call();
    }

    void CallBack::call() const
    {
        m_self->call();
    }

    size_t CallBack::getSenderId() const
    {
        return m_senderId;
    }
}