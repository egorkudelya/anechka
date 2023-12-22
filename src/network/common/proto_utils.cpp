#include "proto_utils.h"

namespace net
{
    namespace detail
    {
        std::vector<std::string> split(const std::string& src, const std::string& delim, bool skipBody)
        {
            std::vector<std::string> res;
            std::string token;
            for (size_t i = 0; i < src.size(); i++)
            {
                bool flag = true;
                for (size_t j = 0; j < delim.size(); j++)
                {
                    if (i + j < src.size() && src[i + j] != delim[j])
                    {
                        flag = false;
                    }
                }
                if (flag && !token.empty())
                {
                    res.push_back(token);
                    token.erase();
                    i += delim.size() - 1;
                    if (skipBody && res.size() == 2)
                    {
                        return res;
                    }
                }
                else
                {
                    token += src[i];
                }
            }
            res.push_back(token);
            return res;
        }

        std::optional<std::string> read(int fd, size_t bytes)
        {
            std::string buffer;
            buffer.reserve(bytes);

            char chunk[detail::ChunkSize] = {0};
            while (buffer.size() < bytes - 1)
            {
                size_t toRead = std::min(detail::ChunkSize, bytes - buffer.size());
                ssize_t r = ::read(fd, chunk, toRead);
                if (r < 0)
                {
                    return {};
                }

                buffer.append(chunk, r);
                memset(chunk, 0, detail::ChunkSize);
            }
            return buffer;
        }

        ssize_t write(int fd, const std::string& buffer)
        {
            return ::write(fd, buffer.data(), buffer.size());
        }
    }

    std::optional<std::string> deserializeHandler(const std::string& serialized)
    {
        auto parts = detail::split(serialized, detail::Boundary, true);
        if (parts.size() != 2)
        {
            return {};
        }
        return parts.at(1);
    }

    std::optional<size_t> deserializeSize(const std::string& serialized)
    {
        size_t res = 0;
        auto parts = detail::split(serialized, "--", true);
        if (parts.empty())
        {
            return {};
        }
        try
        {
            res = std::stoul(parts.at(0));
        }
        catch (const std::exception& err)
        {
            return {};
        }
        return res;
    }
}