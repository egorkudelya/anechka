#include "client.h"

namespace net
{
    ClientStub::ClientStub(int port)
        : AbstractClientStub(port)
    {
    }

    std::tuple<bool, std::string, std::string> ClientStub::RequestTxtFileIndexing(const std::string& path)
    {
        auto requestPtr = std::make_shared<Path::Path>();
        auto responsePtr = std::make_shared<InsertResponse::InsertResponse>();

        requestPtr->getPath() = path;
        execute("RequestTxtFileIndexing", requestPtr, responsePtr);

        return std::make_tuple(responsePtr->getOk(), responsePtr->getTook(), responsePtr->getEnginestatus());
    }

    std::tuple<bool, size_t, std::string, std::string> ClientStub::RequestRecursiveDirIndexing(const std::string& path)
    {
        auto requestPtr = std::make_shared<Path::Path>();
        auto responsePtr = std::make_shared<InsertResponse::InsertResponse>();

        requestPtr->getPath() = path;
        execute("RequestRecursiveDirIndexing", requestPtr, responsePtr);

        return std::make_tuple(responsePtr->getOk(), responsePtr->getIndexsize(), responsePtr->getTook(),
                               responsePtr->getEnginestatus());
    }

    std::tuple<std::string, std::string> ClientStub::RequestTokenSearch(const std::string& token)
    {
        auto requestPtr = std::make_shared<BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<SearchResponse::SearchResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenSearch", requestPtr, responsePtr);

        return std::make_tuple(responsePtr->getJson(), responsePtr->getTook());
    }

    std::tuple<bool, std::string> ClientStub::RequestTokenDeletion(const std::string& token)
    {
        auto requestPtr = std::make_shared<BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<EraseResponse::EraseResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenDeletion", requestPtr, responsePtr);

        return std::make_tuple(responsePtr->getOk(), responsePtr->getTook());
    }

    std::tuple<std::vector<std::string>, std::string>
    ClientStub::RequestTokenSearchWithContext(const std::string& token)
    {
        auto requestPtr = std::make_shared<BasicToken::BasicToken>();
        auto responsePtr = std::make_shared<ContextResponse::ContextResponse>();

        requestPtr->getToken() = token;
        execute("RequestTokenSearchWithContext", requestPtr, responsePtr);

        return std::make_tuple(responsePtr->getResult(), responsePtr->getTook());
    }
}