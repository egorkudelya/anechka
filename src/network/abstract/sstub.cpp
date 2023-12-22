#include "sstub.h"

namespace net::stub
{
    AbstractServerStub::AbstractServerStub()
    {
        release_assert(!s_instance, "Server instance already exists");
        s_instance = this;
        m_queue = std::make_shared<core::SafeQueue<SerializedPacket>>();
    }

    AbstractServerStub::~AbstractServerStub()
    {
        if (!m_isDone)
        {
            shutDown();
        }
    }

    bool AbstractServerStub::run()
    {
        if (!init())
        {
            return false;
        }
        for (size_t i = 0; i < m_threadCount; i++)
        {
            m_threads.emplace_back(&AbstractServerStub::readFrames, this);
        }

        m_acceptThread = std::thread(&AbstractServerStub::acceptFrames, this);
        return true;
    }

    bool AbstractServerStub::init()
    {
        m_serverInfo.sin_family = AF_INET;
        m_serverInfo.sin_port = htons(m_port);

        m_sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_sfd < 0)
        {
            return false;
        }

        m_infoLen = sizeof(m_serverInfo);
        if (bind(m_sfd, reinterpret_cast<sockaddr*>(&m_serverInfo), m_infoLen) < 0)
        {
            return false;
        }
        if (::listen(m_sfd, 0) < 0)
        {
            return false;
        }

        std::cout << "Server listening on port " << m_port << '.' << std::endl;
        return true;
    }

    void AbstractServerStub::shutDown()
    {
        if (m_isDone)
        {
            return;
        }

        m_isDone = true;
        shutdown(m_sfd, SHUT_RDWR);
        close(m_sfd);

        if (m_acceptThread.joinable())
        {
            m_acceptThread.join();
        }

        m_queue->signalAbort();
        for (auto&& thread: m_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    void AbstractServerStub::readFrames()
    {
        while (!m_isDone)
        {
            SerializedPacket packet;
            if (m_queue->pop(packet) && !packet.frame.empty())
            {
                dispatchResponse(route(packet));
            }
        }
    }

    void AbstractServerStub::acceptFrames()
    {
        int cfd;
        sockaddr_in clientInfo{0};
        while ((cfd = accept(m_sfd, reinterpret_cast<sockaddr*>(&clientInfo), &m_infoLen)) >= 0)
        {
            if (m_isDone)
            {
                return;
            }
            /**
            * read the first n bytes of the message, which contain the first
            * part of the buffer and likely some part of the boundary
            */
            auto optHeader = detail::read(cfd, detail::PrefixSize);
            if (!optHeader.has_value())
            {
                m_queue->push(SerializedPacket{cfd, std::string{0, detail::PrefixSize}});
                continue;
            }

            const std::string& header = optHeader.value();

            auto optBufSize = deserializeSize(header);
            if (!optBufSize.has_value() || optBufSize.value() < header.size())
            {
                m_queue->push(SerializedPacket{cfd, std::string{0, detail::PrefixSize}});
                continue;
            }

            size_t remainingSize = optBufSize.value() - header.size();

            auto optBuffer = detail::read(cfd, remainingSize);
            if (!optBuffer.has_value())
            {
                m_queue->push(SerializedPacket{cfd, std::string{0, detail::PrefixSize}});
                continue;
            }

            m_queue->push(SerializedPacket{cfd, std::move(header + optBuffer.value())});
        }
    }

    void AbstractServerStub::dispatchResponse(Packet<ResponsePtr>&& serverPacket)
    {
        if (!serverPacket.fd.has_value())
        {
            return;
        }

        int cfd = serverPacket.fd.value();
        std::string buffer = serialize<ResponsePtr>(serverPacket.handler, serverPacket.body);
        if (detail::write(cfd, buffer) < 0)
        {
            close(cfd);
            return;
        }

        // finished dealing with the request, closing client socket
        close(cfd);
    }

}// namespace stub