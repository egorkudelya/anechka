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

    auto clientStubPtr = std::make_unique<anechka::ClientStub>();
    clientStubPtr->RequestTxtFileIndexing("../test/data/sample.txt");

    std::vector<std::string> tokens{"The", "more", "I", "read", "acquire", "certain", "that", "know", "nothing"};

    std::vector<std::thread> clientThreads;
    std::atomic<size_t> responseCount{0};
    for (size_t i = 0; i < 15; i++)
    {
         clientThreads.emplace_back([&tokens, &responseCount] {
            auto clientStubPtr = std::make_unique<anechka::ClientStub>();

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

TEST(Anechka, ContextSearchTest)
{
    /**
     * The test assumes that there is a lot of documents containing the word "America" in directory vault/test
     */
    const std::filesystem::path path = "../vault/imdb/test";
    ASSERT_TRUE(std::filesystem::exists(path));

    auto anechkaPtr = std::make_unique<anechka::Anechka>("../test/config/config.json");
    anechkaPtr->run();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto clientStubPtr = std::make_unique<anechka::ClientStub>();
    clientStubPtr->RequestRecursiveDirIndexing(path);

    std::vector<std::thread> clientThreads;
    std::atomic<int64_t> resSize = -1;
    for (size_t i = 0; i < 15; i++)
    {
         clientThreads.emplace_back([&resSize] {
             auto clientStubPtr = std::make_unique<anechka::ClientStub>();
             for (size_t i = 0; i < 1000; i++)
             {
                 auto res = clientStubPtr->RequestTokenSearchWithContext("America");
                 if (resSize == -1)
                 {
                     // first response
                     resSize = res->getResponses().size();
                 }
                 EXPECT_EQ(res->getResponses().size(), resSize);
                 ASSERT_TRUE(resSize > 0);
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

TEST(Anechka, Comprehensive)
{
    const std::filesystem::path path = "../vault/imdb/test/neg";
    ASSERT_TRUE(std::filesystem::exists(path));

    auto anechkaPtr = std::make_unique<anechka::Anechka>("../test/config/config.json");
    anechkaPtr->run();

    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto clientStubPtr = std::make_unique<anechka::ClientStub>();
    clientStubPtr->RequestRecursiveDirIndexing(path);

    std::vector<std::string> corpus {
        "The", "more", "I", "read", "acquire", "certain", "that", "know", "nothing",
        "apple", "America", "California", "sad", "good", "happy", "shiny", "sun", "moon",
        "earth"
    };

    std::vector<std::thread> clientThreads;
    for (size_t i = 0; i < 10; i++)
    {
         clientThreads.emplace_back([] {
             auto clientStubPtr = std::make_unique<anechka::ClientStub>();
             for (size_t i = 0; i < 5; i++)
             {
                 clientStubPtr->RequestRecursiveDirIndexing("../vault/imdb/test");
             }
         });
    }

    for (size_t i = 0; i < 5; i++)
    {
         clientThreads.emplace_back([] {
             auto clientStubPtr = std::make_unique<anechka::ClientStub>();
             for (size_t i = 0; i < 100; i++)
             {
                 auto r = clientStubPtr->RequestTokenSearchWithContext("sun");
//                 ASSERT_TRUE(!r->getResult().empty());
                 ASSERT_TRUE(r->getStatus() == net::ProtocolStatus::OK);
             }
         });
    }

    for (size_t i = 0; i < 10; i++)
    {
         clientThreads.emplace_back([&corpus] {
             auto clientStubPtr = std::make_unique<anechka::ClientStub>();
             for (size_t i = 0; i < 50; i++)
             {
                 std::vector<std::string> sample;
                 std::sample(
                     corpus.begin(),
                     corpus.end(),
                     std::back_inserter(sample),
                     corpus.size()/2,
                     std::mt19937{std::random_device{}()}
                 );

                 for (const std::string& token: sample)
                 {
                     auto r = clientStubPtr->RequestTokenDeletion(token);
                     ASSERT_TRUE(r->getStatus() == net::ProtocolStatus::OK);
                 }
             }
         });
    }

    for (size_t i = 0; i < 5; i++)
    {
         clientThreads.emplace_back([&corpus] {
             auto clientStubPtr = std::make_unique<anechka::ClientStub>();
             for (size_t i = 0; i < 100; i++)
             {
                 std::vector<std::string> sample;
                 std::sample(
                     corpus.begin(),
                     corpus.end(),
                     std::back_inserter(sample),
                     3,
                     std::mt19937{std::random_device{}()}
                 );

                 std::string query;
                 for (const std::string& token: sample)
                 {
                     query += token + ' ';
                 }
                 auto r = clientStubPtr->RequestQuerySearch(query);
                 ASSERT_TRUE(r->getStatus() == net::ProtocolStatus::OK);
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