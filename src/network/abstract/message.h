#pragma once

#include "../utility/common.h"
#include <memory>
#include <string>

namespace net
{
    enum class ProtocolStatus
    {
        OK = 0,
        ProtocolError = 1,
    };

    struct MessageMetadata
    {
        Json toJson() const;
        bool fromJson(const Json& json);

        ProtocolStatus status{ProtocolStatus::OK};
        std::string meta{"OK"};
    };

    class AbstractMessage
    {
    public:
        virtual ~AbstractMessage() = default;
        virtual Json toJson() const = 0;
        virtual void fromJson(const Json& json) = 0;
        void print() const;
        void setMetadata(ProtocolStatus status, const std::string& meta);
        void setMetadata(MessageMetadata metadata);
        MessageMetadata getMetadata() const;
        ProtocolStatus getStatus() const;

    protected:
        MessageMetadata m_metadata{};
    };

    using RequestPtr = std::shared_ptr<AbstractMessage>;
    using ResponsePtr = std::shared_ptr<AbstractMessage>;
}