#pragma once

#include <filesystem>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

class MMapASCII
{
public:
    explicit MMapASCII(const std::string& path);
    ~MMapASCII();
    MMapASCII(MMapASCII&& other) noexcept = default;
    MMapASCII& operator=(MMapASCII&& other) noexcept = default;
    char operator[](size_t i) const noexcept;
    size_t size() const noexcept;

    inline auto cbegin() const noexcept
    {
        return m_view.cbegin();
    }

    inline auto cend() const noexcept
    {
        return m_view.cend();
    }

    inline auto begin() const noexcept
    {
        return cbegin();
    }

    inline auto end() const noexcept
    {
        return cend();
    }

private:
    int m_fd;
    size_t m_fileSize;
    char* m_internal;
    std::string_view m_view;
};