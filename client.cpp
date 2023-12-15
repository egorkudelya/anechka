#include "../client/client.h"


int main()
{
    auto clientStubPtr = std::make_unique<net::ClientStub>();

    utils::print(clientStubPtr->RequestRecursiveDirIndexing("../vault/"));
    utils::print(clientStubPtr->RequestTokenDeletion("orange"));
    utils::print(clientStubPtr->RequestTokenSearch("apple"));
    utils::print(clientStubPtr->RequestTokenSearchWithContext("America"));

    return 0;
}