#include <gtest/gtest.h>
#include <thread>
#include <random>
#include "../src/server/server.h"
#include "../src/client/client.h"


TEST(Anechka, Async)
{
    auto anechkaPtr = std::make_unique<anechka::Anechka>("../test/config/config.json");
    anechkaPtr->run();

    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto clientStubPtr = std::make_unique<net::ClientStub>();
    clientStubPtr->RequestTxtFileIndexing("../test/data/sample.txt");

    std::vector<std::string> tokens{"The", "more", "I", "read", "acquire", "certain", "that", "know", "nothing"};

    std::vector<std::thread> clientThreads;
    std::atomic<size_t> responseCount{0};
    for (size_t i = 0; i < 15; i++)
    {
         clientThreads.emplace_back([&tokens, &responseCount] {
            auto clientStubPtr = std::make_unique<net::ClientStub>();

            std::vector<std::string> out;
            std::sample(
                tokens.cbegin(),
                tokens.cend(),
                std::back_inserter(out),
                tokens.size(),
                std::mt19937{std::random_device{}()}
            );

            for (size_t i = 0; i < 1000; i++)
            {
                auto r1 = clientStubPtr->RequestTokenSearch(out[i % out.size()]);
                auto r2 = clientStubPtr->RequestTokenSearchWithContext(out[i % out.size()]);
                if (r1->getStatus() == net::ProtocolStatus::OK &&
                    r2->getStatus() == net::ProtocolStatus::OK)
                {
                    responseCount++;
                }
            }
        });
    }

    for (auto&& thread: clientThreads)
    {
         if (thread.joinable())
         {
             thread.join();
         }
    }

    anechkaPtr->shutDown();
    EXPECT_EQ(responseCount, 1000 * 15);
}

TEST(Anechka, LoadTest)
{
    /**
     * The test assumes that there is a lot of documents containing the word "America" in directory vault/test
     */
    const std::filesystem::path path = "../vault/test";
    ASSERT_TRUE(std::filesystem::exists(path));

    auto anechkaPtr = std::make_unique<anechka::Anechka>("../test/config/config.json");
    anechkaPtr->run();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto clientStubPtr = std::make_unique<net::ClientStub>();
    clientStubPtr->RequestRecursiveDirIndexing(path);

    std::vector<std::thread> clientThreads;
    std::atomic<ssize_t> resSize = -1;
    for (size_t i = 0; i < 15; i++)
    {
         clientThreads.emplace_back([&resSize] {
             auto clientStubPtr = std::make_unique<net::ClientStub>();
             for (size_t i = 0; i < 1000; i++)
             {
                 auto res = clientStubPtr->RequestTokenSearchWithContext("America");
                 if (resSize == -1)
                 {
                     // first response
                     resSize = res->getResult().size();
                 }
                 ASSERT_TRUE(res->getResult().size() == resSize && resSize > 0);
             }
         });
    }

    for (auto&& thread: clientThreads)
    {
         if (thread.joinable())
         {
             thread.join();
         }
    }

    anechkaPtr->shutDown();
}