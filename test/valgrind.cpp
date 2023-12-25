#include <gtest/gtest.h>
#include <thread>
#include <random>
#include "../src/server/server.h"
#include "../src/client/client.h"


TEST(Anechka, Valgrind)
{
    auto anechkaPtr = std::make_unique<anechka::Anechka>("../test/config/config.json");
    anechkaPtr->run();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto clientStubPtr = std::make_unique<anechka::ClientStub>();

    const std::string& source = "../test/data/sample.txt";
    clientStubPtr->RequestTxtFileIndexing(source);

    std::vector<std::string> tokens{"The", "more", "I", "read", "acquire", "certain", "that", "know", "nothing"};

    std::vector<std::thread> clientThreads;
    std::atomic<size_t> responseCount{0};
    for (size_t i = 0; i < 10; i++)
    {
        clientThreads.emplace_back([&tokens, &responseCount, &source]{
            auto clientStubPtr = std::make_unique<anechka::ClientStub>();

            std::vector<std::string> out;
            std::sample(
                tokens.cbegin(),
                tokens.cend(),
                std::back_inserter(out),
                tokens.size(),
                std::mt19937{std::random_device{}()}
            );

            for (size_t i = 0; i < 100; i++)
            {
                auto r1 = clientStubPtr->RequestTokenSearch(out[i % out.size()]);
                auto r2 = clientStubPtr->RequestTokenSearchWithContext(out[i % out.size()]);
                auto r3 = clientStubPtr->RequestTokenDeletion(out[i % out.size()]);
                auto r4 = clientStubPtr->RequestTxtFileIndexing(source);
                auto r5 = clientStubPtr->RequestTokenSearchWithContext(out[0] + ' ' + out[1] + ' ' + out[2]);
                if (r1->getStatus() == net::ProtocolStatus::OK &&
                    r2->getStatus() == net::ProtocolStatus::OK &&
                    r3->getStatus() == net::ProtocolStatus::OK &&
                    r4->getStatus() == net::ProtocolStatus::OK &&
                    r5->getStatus() == net::ProtocolStatus::OK)
                {
                    responseCount++;
                }
//                r1->print();
//                r2->print();
//                r3->print();
            }
        });
    }

    auto res = clientStubPtr->RequestTokenDeletion("acquire");
    ASSERT_TRUE(res->getOk());

    for (auto&& thread: clientThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    anechkaPtr->shutDown();
    EXPECT_EQ(responseCount, 10 * 100);
}