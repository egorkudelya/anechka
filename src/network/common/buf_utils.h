#pragma once

#include "../abstract/message.h"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <optional>
#include <charconv>

namespace net
{
    namespace detail
    {
        inline const std::string boundary = "--RF@B&@*HJNSINc8nKLnddHQ--";

        // TODO a header larger than 20 bytes would crash the server, gracefully discard such packets

        // provide support up to the largest 64-bit number
        const size_t hsize = 20;
        const size_t csize = 1024;

        void removeLeading(std::string& str, char charToRemove);
        std::vector<std::string> split(const std::string& src, const std::string& delim, bool skipBody = false);
    }

    template<typename MessagePtr>
    struct Packet
    {
        std::optional<int> fd;
        std::optional<std::string> size;
        std::string handler;
        MessagePtr body;
    };

    struct SerializedPacket
    {
        int fd;
        std::string frame;
    };

    std::string serialize(const std::string& handlerName, const std::string& body);

    template<typename MessagePtr>
    Packet<MessagePtr> deserialize(const std::string& serialized)
    {
        using ElementType = typename MessagePtr::element_type;
        static_assert(std::is_base_of_v<AbstractMessage, ElementType>);

        auto parts = detail::split(serialized, detail::boundary);
        release_assert(parts.size() == 3, "Tried to deserialize a corrupted packet");

        auto messagePtr = std::make_shared<ElementType>();

        std::string& body = parts[2];
        messagePtr->fromJson(Json::parse(body));

        return Packet<MessagePtr>{{}, parts[0], parts[1], messagePtr};
    }

    std::string deserializeHandler(const std::string& serialized);

    size_t deserializeSize(const std::string& serialized);
}
