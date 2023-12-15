#include <gtest/gtest.h>
#include <thread>
#include <random>
#include "../src/server/server.h"
#include "../src/client/client.h"


TEST(Anechka, Valgrind)
{
    auto anechkaPtr = std::make_unique<anechka::Anechka>("../test/config/config.json");
    anechkaPtr->run();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    auto clientStubPtr = std::make_unique<net::ClientStub>();

    const std::string& source = "../test/data/sample.txt";
    clientStubPtr->RequestTxtFileIndexing(source);

    std::vector<std::string> tokens{"The", "more", "I", "read", "acquire", "certain", "that", "know", "nothing"};

    std::vector<std::thread> clientThreads;
    std::atomic<size_t> responseCount{0};
    for (size_t i = 0; i < 10; i++)
    {
        clientThreads.emplace_back([&tokens, &responseCount, &source]{
            auto clientStubPtr = std::make_unique<net::ClientStub>();

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
                auto r1 = std::get<0>(clientStubPtr->RequestTokenSearch(out[i % out.size()]));
                auto r2 = std::get<1>(clientStubPtr->RequestTokenSearchWithContext(out[i % out.size()]));
                auto r3 = std::get<0>(clientStubPtr->RequestTxtFileIndexing(source));
                if (!r1.empty() && !r2.empty() && r3)
                {
                    responseCount++;
                }
            }
        });
    }

    auto[ok, stat] = clientStubPtr->RequestTokenDeletion("acquire");
    ASSERT_TRUE(ok);


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