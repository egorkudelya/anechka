#pragma once

#include "message.h"
#include "../common/proto_utils.h"

namespace net::stub
{
    class AbstractClientStub
    {
    public:
        explicit AbstractClientStub(int port = 4444);
        virtual ~AbstractClientStub() = default;

    protected:
        template <typename T>
        bool execute(const std::string& handlerName, const RequestPtr& requestPtr, T& responsePtr)
        {
            release_assert(servConnect(), "Client failed to connect to server");

            std::string requestBuffer = serialize<RequestPtr>(handlerName, requestPtr);
            if (detail::write(m_sfd, requestBuffer) < 0)
            {
                closeConnection();
                return false;
            }
            /**
            * read the first n bytes of the message, which contain the first
            * part of the buffer and likely some part of the boundary
            */
            auto optHeader = detail::read(m_sfd, detail::PrefixSize);
            if (!optHeader.has_value())
            {
                closeConnection();
                return false;
            }

            const std::string& header = optHeader.value();
            auto optBufSize = deserializeSize(header);
            if (!optBufSize.has_value() || optBufSize.value() < header.size())
            {
                closeConnection();
                return false;
            }

            size_t remainingSize = optBufSize.value() - header.size();
            auto optBuffer = detail::read(m_sfd, remainingSize);
            if (!optBuffer.has_value())
            {
                closeConnection();
                return false;
            }

            const Packet<T> packet = deserialize<T>(header +  optBuffer.value());
            responsePtr = packet.body;

            closeConnection();
            return true;
        }

    private:
        bool servConnect();
        void closeConnection();

    private:
        int m_sfd;
        sockaddr_in m_serverInfo{0};
        socklen_t m_servInfoLen{0};
    };

}// namespace stub