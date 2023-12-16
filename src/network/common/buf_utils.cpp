#include "buf_utils.h"

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
    }

    std::string serialize(const std::string& handlerName, const std::string& body)
    {
        size_t hbSize = handlerName.size() + body.size() + detail::boundary.size() * 2;
        size_t sSize = std::to_string(hbSize).size();

        // +1 covers \0
        return {std::to_string(hbSize + sSize + 1) + detail::boundary + handlerName + detail::boundary + body};
    }

    std::string deserializeHandler(const std::string& serialized)
    {
        auto parts = detail::split(serialized, detail::boundary, true);
        release_assert(parts.size() == 2, "Tried to deserialize a corrupted packet");

        return parts[1];
    }

    size_t deserializeSize(const std::string& serialized)
    {
        size_t res = 0;
        auto parts = detail::split(serialized, "--", true);
        release_assert(!parts.empty(), "Tried to deserialize a corrupted packet");

        const std::string& sizeStr = parts[0];
        std::from_chars(sizeStr.data(), sizeStr.data() + sizeStr.size(), res);

        return res;
    }
}