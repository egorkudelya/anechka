#include "message.h"

namespace net
{
    Json MessageMetadata::toJson() const
    {
        return {{"status", (int)status}, {"meta", meta}};
    }

    bool MessageMetadata::fromJson(const Json& json)
    {
        try
        {
            status = json.at("status").get<ProtocolStatus>();
            meta = json.at("meta");
        }
        catch (const std::exception& err)
        {
            return false;
        }
        return true;
    }

    MessageMetadata AbstractMessage::getMetadata() const
    {
        return m_metadata;
    }

    ProtocolStatus AbstractMessage::getStatus() const
    {
        return m_metadata.status;
    }

    void AbstractMessage::setMetadata(ProtocolStatus status, const std::string& meta)
    {
        m_metadata = MessageMetadata{status, meta};
    }

    void AbstractMessage::setMetadata(MessageMetadata metadata)
    {
        m_metadata = std::move(metadata);
    }
}