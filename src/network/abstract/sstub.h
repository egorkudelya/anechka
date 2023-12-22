#pragma once

#include "../common/proto_utils.h"
#include "message.h"
#include "safe_queue.h"
#include "xxh64_hasher.h"
#include <memory>
#include <thread>
#include <vector>

namespace net::stub
{
    inline constexpr uint64_t hash(std::string_view data)
    {
        return xxh64::hash(data.data(), data.size(), 0);
    }

    class AbstractServerStub
    {
    public:
        AbstractServerStub();
        virtual ~AbstractServerStub();
        bool run();
        void shutDown();

    protected:
        bool init();
        virtual Packet<ResponsePtr> route(const SerializedPacket& frame) = 0;
        void dispatchResponse(Packet<ResponsePtr>&& serverFrame);
        void acceptFrames();
        void readFrames();

    protected:
        int m_sfd;
        int m_port;
        size_t m_threadCount;
        bool m_isDone{false};
        socklen_t m_infoLen;
        sockaddr_in m_serverInfo{};
        std::vector<std::thread> m_threads;
        std::thread m_acceptThread;
        core::SafeQueuePtr<SerializedPacket> m_queue;
        inline static AbstractServerStub* s_instance{nullptr};
    };

    using ServerStubPtr = std::shared_ptr<AbstractServerStub>;

}// namespace stub