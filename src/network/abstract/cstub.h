#pragma once

#include "message.h"
#include "../common/buf_utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>


namespace net::stub
{
    class AbstractClientStub
    {
    public:
        explicit AbstractClientStub(int port=4444);
        virtual ~AbstractClientStub() = default;

    protected:
        template <typename RResponsePtr>
        bool execute(const std::string& handlerName, const RequestPtr& requestPtr, RResponsePtr& responsePtr)
        {
            release_assert(servConnect(), "Client failed to connect to server");

            std::string serializedRequest = requestPtr->toJson().dump();
            std::string requestBuffer = serialize(handlerName, serializedRequest);
            if (write(m_sfd, requestBuffer.c_str(), requestBuffer.size()) < 0)
            {
                closeConnection();
                return false;
            }
            /**
            * read the first n bytes of the message, which contain the first
            * part of the buffer and likely some part of the boundary
            */
            std::string header(detail::hsize, 0);
            if (read(m_sfd, header.data(), header.size()) < 0)
            {
                closeConnection();
                return false;
            }
            utils::removeLeading(header, '\0');

            size_t bufferSize = deserializeSize(header);
            size_t remainingSize = bufferSize - header.size();

            std::string buffer;
            buffer.reserve(remainingSize);

            char chunk[detail::csize] = {0};
            while (buffer.size() < remainingSize - 1)
            {
                size_t toRead = std::min(detail::csize, remainingSize - buffer.size());
                ssize_t r = read(m_sfd, chunk, toRead);
                if (r < 0)
                {
                    closeConnection();
                    return false;
                }

                buffer.append(chunk, r);
                memset(chunk, 0, detail::csize);
            }

            auto packet = deserialize<RResponsePtr>(header + buffer);
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