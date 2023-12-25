#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <random>
#include "../src/engine/doc_trace.h"

TEST(DocTrace, Basic)
{
    core::DocTrace docTrace{10};
    docTrace.addOrIncrement("first.txt", {0});
    docTrace.addOrIncrement("second.txt", {0});
    docTrace.addOrIncrement("second.txt", {0});
    docTrace.addOrIncrement("second.txt", {0});
    docTrace.addOrIncrement("third.txt", {0});

    EXPECT_EQ(docTrace.getRefCount("first.txt"), 1);
    EXPECT_EQ(docTrace.getRefCount("second.txt"), 3);
    EXPECT_EQ(docTrace.getRefCount("third.txt"), 1);

    docTrace.eraseOrDecrement("second.txt");
    EXPECT_EQ(docTrace.getRefCount("second.txt"), 2);

    docTrace.eraseOrDecrement("second.txt");
    EXPECT_EQ(docTrace.getRefCount("second.txt"), 1);

    docTrace.eraseOrDecrement("second.txt");
    EXPECT_EQ(docTrace.getRefCount("second.txt"), 0);

    docTrace.addOrIncrement("second.txt", {0});
    EXPECT_EQ(docTrace.getRefCount("second.txt"), 1);
}

TEST(DocTrace, Async)
{
    core::DocTrace docTrace{10};
    std::vector<std::string> corpus {
        "first.txt", "second.txt", "third.txt", "fourth.txt", "fifth.txt",
        "sixth.txt", "seventh.txt", "eighth.txt", "ninth.txt", "tenth.txt"
    };

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(0, corpus.size() - 1);

    std::vector<std::thread> clientThreads;
    for (size_t i = 0; i < 5; i++)
    {
        for (size_t j = 0; j < 100; j++)
        {
            clientThreads.emplace_back([&corpus, &docTrace, &uni, &rng]{
                docTrace.addOrIncrement(corpus[uni(rng)], {0});
            });
        }
    }

    for (size_t i = 0; i < 5; i++)
    {
        for (size_t j = 0; j < 100; j++)
        {
            clientThreads.emplace_back([&corpus, &docTrace, &uni, &rng] {
                docTrace.eraseOrDecrement(corpus[uni(rng)]);
            });
        }
    }

    for (auto&& doc: corpus)
    {
        ASSERT_TRUE(docTrace.getRefCount(doc) >= 0);
    }

    for (auto&& thread: clientThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}