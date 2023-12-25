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

    void AbstractMessage::print() const
    {
        std::string result;
        const std::string& src = toJson().dump();
        for (size_t i = 0; i < src.size(); i++)
        {
            if (src[i] == '\\' && i + 1 < src.size())
            {
                switch (src[i + 1])
                {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case '\\': {if (i + 2 < src.size() && src[i + 2] == 'n'){continue;}break;}
                    default: result += src[i + 1];
                }
                i++;
            }
            else if (src[i] == '{' && (i + 2 < src.size() && src[i + 2] != 'n'))
            {
                result += "{\n";
            }
            else if ((src[i] == '\"' && (i + 1 < src.size() && (src[i + 1] == '[' || src[i + 1] == '{'))))
            {
                result += src[i + 1];
                i++;
            }
            else if ((src[i] == '}' || src[i] == ']'))
            {
                if (i + 1 < src.size() && src[i + 1] == '\"')
                {
                    result += src[i];
                    i++;
                }
                else if (i + 1 == src.size())
                {
                    result += '\n';
                    result += src[i];
                }
                else
                {
                    result += src[i];
                }
            }
            else
            {
                result += src[i];
            }
        }
        std::cout << result << '\n';
    }
}