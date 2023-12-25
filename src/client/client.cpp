#include "client.h"

namespace anechka
{
    ClientStub::ClientStub(int port)
        : AbstractClientStub(port)
    {
    }

    net::InsertResponse::ResponsePtr ClientStub::RequestTxtFileIndexing(const std::string& path)
    {
        auto requestPtr = std::make_shared<net::Path::Path>();
        auto responsePtr = std::make_shared<net::InsertResponse::InsertResponse>();

        requestPtr->getPath() = path;
        execute("RequestTxtFileIndexing", requestPtr, responsePtr);

        return responsePtr;
    }

    net::InsertResponse::ResponsePtr ClientStub::RequestRecursiveDirIndexing(const std::string& path)
    {
        auto requestPtr = std::make_shared<net::Path::Path>();
        auto responsePtr = std::make_shared<net::InsertResponse::InsertResponse>();

        requestPtr->getPath() = path;
        execute("RequestRecursiveDirIndexing", requestPtr, responsePtr);

        return responsePtr;
    }

    net::SearchResponse::ResponsePtr ClientStub::RequestTokenSearch(const std::string& token)
    {
        auto requestPtr = std::make_shared<net::BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<net::SearchResponse::SearchResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenSearch", requestPtr, responsePtr);

        return responsePtr;
    }

    net::EraseResponse::ResponsePtr ClientStub::RequestTokenDeletion(const std::string& token)
    {
        auto requestPtr = std::make_shared<net::BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<net::EraseResponse::EraseResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenDeletion", requestPtr, responsePtr);

        return responsePtr;
    }

    net::ContextSearchResponse::ResponsePtr ClientStub::RequestTokenSearchWithContext(const std::string& token)
    {
        auto requestPtr = std::make_shared<net::BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<net::ContextSearchResponse::ContextSearchResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenSearchWithContext", requestPtr, responsePtr);

        return responsePtr;
    }

    net::SearchQueryResponse::ResponsePtr ClientStub::RequestQuerySearch(const std::string& query)
    {
        auto requestPtr = std::make_shared<net::SearchQueryRequest::SearchQueryRequest>();
        auto responsePtr = std::make_shared<net::SearchQueryResponse::SearchQueryResponse>();

        requestPtr->getQuery() = query;
        execute("RequestQuerySearch", requestPtr, responsePtr);

        return responsePtr;
    }
}