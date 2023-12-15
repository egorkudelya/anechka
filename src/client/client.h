#pragma once

#include "../network/abstract/cstub.h"
#include "../network/gen/gen_messages.h"

namespace net
{
    class ClientStub: public stub::AbstractClientStub
    {
    public:
        explicit ClientStub(int port=4444);
        virtual ~ClientStub() = default;

        std::tuple<bool, std::string, std::string> RequestTxtFileIndexing(const std::string& path);
        std::tuple<bool, size_t, std::string, std::string> RequestRecursiveDirIndexing(const std::string& path);
        std::tuple<std::string, std::string> RequestTokenSearch(const std::string& token);
        std::tuple<bool, std::string> RequestTokenDeletion(const std::string& token);
        std::tuple<std::vector<std::string>, std::string> RequestTokenSearchWithContext(const std::string& token);
    };
}