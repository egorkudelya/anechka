#include "timer.h"

namespace utils
{
    Timer::Timer()
        : m_start{std::chrono::high_resolution_clock::now()}
    {
    }

    Timer::Timer(Timer&& other) noexcept
        : m_start{other.m_start}
    {
    }

    uint64_t Timer::getInterval() const
    {
        using namespace std::chrono;
        const auto end = high_resolution_clock::now();
        return duration_cast<duration<uint64_t, std::milli>>(end - m_start).count();
    }
}