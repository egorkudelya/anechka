#include "../client/client.h"


int main()
{
    auto clientStubPtr = std::make_unique<anechka::ClientStub>();

    clientStubPtr->RequestRecursiveDirIndexing("../vault/")->print();
    clientStubPtr->RequestTokenSearchWithContext("America")->print();
    clientStubPtr->RequestTokenDeletion("orange")->print();
    clientStubPtr->RequestTokenDeletion("apple")->print();
    clientStubPtr->RequestTokenSearch("good")->print();
    clientStubPtr->RequestQuerySearch("it was a good movie")->print();

    return 0;
}