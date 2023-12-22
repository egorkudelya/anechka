#pragma once

#include "../abstract/message.h"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <optional>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace net
{
    namespace detail
    {
        const size_t PrefixSize = 20;
        const size_t ChunkSize = 1024;
        const std::string Boundary = "--RF@B&@*HJNSINc8nKLnddHQ--";

        std::optional<std::string> read(int fd, size_t bytes);
        ssize_t write(int fd, const std::string& buffer);

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

    template<typename MessagePtr>
    std::string serialize(const std::string& handlerName, const MessagePtr& messagePtr)
    {
        if (!messagePtr)
        {
            MessageMetadata metadata{ProtocolStatus::ProtocolError, "Corrupted packet"};
            const std::string& meta = metadata.toJson().dump();

            return {
                std::to_string(detail::Boundary.size() * 3 + meta.size() + 1) + detail::Boundary +
                "" + detail::Boundary +
                "" + detail::Boundary +
                meta
            };
        }
        std::string body, metadata;
        try
        {
            body = messagePtr->toJson().dump();
            metadata = messagePtr->getMetadata().toJson().dump();
        }
        catch (const std::exception& err)
        {
            metadata = MessageMetadata{ProtocolStatus::ProtocolError, "Corrupted message"}.toJson().dump();
        }

        size_t fsize = handlerName.size() + body.size() + metadata.size() + detail::Boundary.size() * 3;
        size_t ssize = std::to_string(fsize).size();

        // +1 covers \0
        return {
            std::to_string(fsize + ssize + 1) + detail::Boundary +
            handlerName + detail::Boundary +
            body + detail::Boundary +
            metadata
        };
    }

    template<typename MessagePtr>
    Packet<MessagePtr> deserialize(const std::string& serialized)
    {
        using MessageType = typename MessagePtr::element_type;
        static_assert(std::is_base_of_v<AbstractMessage, MessageType>);

        auto messagePtr = std::make_shared<MessageType>();
        auto parts = detail::split(serialized, detail::Boundary);
        if (parts.size() != 4)
        {
            messagePtr->setMetadata(ProtocolStatus::ProtocolError, "Internal protocol error");
            return Packet<MessagePtr>{{}, {}, {}, messagePtr};
        }

        MessageMetadata metadata;
        metadata.fromJson(parts[3]);

        messagePtr->fromJson(Json::parse(parts.at(2)));
        messagePtr->setMetadata(metadata);

        return Packet<MessagePtr>{{}, parts[0], parts[1], messagePtr};
    }

    std::optional<std::string> deserializeHandler(const std::string& serialized);
    std::optional<size_t> deserializeSize(const std::string& serialized);
}
