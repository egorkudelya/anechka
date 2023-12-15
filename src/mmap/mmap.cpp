#include "mmap.h"

MMapASCII::MMapASCII(const std::string& path)
{
    if ((m_fd = open(path.c_str(), O_RDONLY)) == -1)
    {
        throw std::invalid_argument("Invalid filepath");
    }

    m_fileSize = std::filesystem::file_size(path);
    if (m_fileSize == 0)
    {
        return;
    }

    m_internal = static_cast<char*>(mmap(nullptr, m_fileSize, PROT_READ, MAP_SHARED, m_fd, 0));
    if (m_internal == MAP_FAILED)
    {
        close(m_fd);
        throw std::runtime_error("Failed to map resource");
    }

    m_view = m_internal;
}

MMapASCII::~MMapASCII()
{
    munmap(m_internal, m_fileSize);
    close(m_fd);
}

char MMapASCII::operator[](size_t i) const noexcept
{
    return m_view[i];
}

size_t MMapASCII::size() const noexcept
{
    return m_view.size();
}