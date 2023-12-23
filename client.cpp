#include "../client/client.h"


int main()
{
    auto clientStubPtr = std::make_unique<net::ClientStub>();

    auto r1 = clientStubPtr->RequestRecursiveDirIndexing("../vault/");
    auto r2 = clientStubPtr->RequestTokenSearchWithContext("America");
    auto r3 = clientStubPtr->RequestTokenDeletion("orange");
    auto r4 = clientStubPtr->RequestTokenDeletion("apple");
    auto r5 = clientStubPtr->RequestTokenSearch("good");
    auto r6 = clientStubPtr->RequestTokenSearchWithContext("conscientious");

    r1->print();
    r2->print();
    r3->print();
    r4->print();
    r5->print();
    r6->print();

    return 0;
}