#pragma once

#include <chrono>

namespace utils
{
    class Timer
    {
    public:
        Timer();
        Timer(Timer&& other) noexcept;
        uint64_t getInterval() const;

    private:
        const std::chrono::high_resolution_clock::time_point m_start;
    };
}