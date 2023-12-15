#include "cstub.h"

namespace net::stub
{
    AbstractClientStub::AbstractClientStub(int port)
    {
        m_serverInfo.sin_family = AF_INET;
        m_serverInfo.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        m_serverInfo.sin_port = htons(port);
        m_servInfoLen = sizeof(m_serverInfo);
    }

    bool AbstractClientStub::servConnect()
    {
        m_sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_sfd < 0)
        {
            return false;
        }
        if (connect(m_sfd, reinterpret_cast<sockaddr*>(&m_serverInfo), m_servInfoLen) < 0)
        {
            return false;
        }
        return true;
    }

    void AbstractClientStub::closeConnection()
    {
        shutdown(m_sfd, SHUT_WR);
        close(m_sfd);
    }

}
