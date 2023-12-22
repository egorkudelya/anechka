#pragma once

#include "../network/abstract/cstub.h"
#include "../network/gen/gen_messages.h"

namespace net
{
    class ClientStub: public stub::AbstractClientStub
    {
    public:
        explicit ClientStub(int port = 4444);
        virtual ~ClientStub() = default;

        InsertResponse::ResponsePtr RequestTxtFileIndexing(const std::string& path);
        InsertResponse::ResponsePtr RequestRecursiveDirIndexing(const std::string& path);
        SearchResponse::ResponsePtr RequestTokenSearch(const std::string& token);
        EraseResponse::ResponsePtr RequestTokenDeletion(const std::string& token);
        ContextResponse::ResponsePtr RequestTokenSearchWithContext(const std::string& token);
    };
}