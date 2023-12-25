#pragma once

#include "../network/abstract/cstub.h"
#include "../network/gen/gen_messages.h"

namespace anechka
{
    class ClientStub: public net::stub::AbstractClientStub
    {
    public:
        explicit ClientStub(int port = 4444);
        virtual ~ClientStub() = default;

        net::InsertResponse::ResponsePtr RequestTxtFileIndexing(const std::string& path);
        net::InsertResponse::ResponsePtr RequestRecursiveDirIndexing(const std::string& path);
        net::SearchResponse::ResponsePtr RequestTokenSearch(const std::string& token);
        net::EraseResponse::ResponsePtr RequestTokenDeletion(const std::string& token);
        net::ContextSearchResponse::ResponsePtr RequestTokenSearchWithContext(const std::string& token);
        net::SearchQueryResponse::ResponsePtr RequestQuerySearch(const std::string& query);
    };
}