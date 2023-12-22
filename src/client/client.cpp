#include "client.h"

namespace net
{
    ClientStub::ClientStub(int port)
        : AbstractClientStub(port)
    {
    }

    InsertResponse::ResponsePtr ClientStub::RequestTxtFileIndexing(const std::string& path)
    {
        auto requestPtr = std::make_shared<Path::Path>();
        auto responsePtr = std::make_shared<InsertResponse::InsertResponse>();

        requestPtr->getPath() = path;
        execute("RequestTxtFileIndexing", requestPtr, responsePtr);

        return responsePtr;
    }

    InsertResponse::ResponsePtr ClientStub::RequestRecursiveDirIndexing(const std::string& path)
    {
        auto requestPtr = std::make_shared<Path::Path>();
        auto responsePtr = std::make_shared<InsertResponse::InsertResponse>();

        requestPtr->getPath() = path;
        execute("RequestRecursiveDirIndexing", requestPtr, responsePtr);

        return responsePtr;
    }

    SearchResponse::ResponsePtr ClientStub::RequestTokenSearch(const std::string& token)
    {
        auto requestPtr = std::make_shared<BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<SearchResponse::SearchResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenSearch", requestPtr, responsePtr);

        return responsePtr;
    }

    EraseResponse::ResponsePtr ClientStub::RequestTokenDeletion(const std::string& token)
    {
        auto requestPtr = std::make_shared<BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<EraseResponse::EraseResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenDeletion", requestPtr, responsePtr);

        return responsePtr;
    }

    ContextResponse::ResponsePtr ClientStub::RequestTokenSearchWithContext(const std::string& token)
    {
        auto requestPtr = std::make_shared<BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<ContextResponse::ContextResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenSearchWithContext", requestPtr, responsePtr);

        return responsePtr;
    }
}