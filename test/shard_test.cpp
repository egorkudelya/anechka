#include <gtest/gtest.h>
#include "../src/engine/shard.h"

TEST(ShardTest, Basic)
{
    const float loadFactor = 0.75;
    const float estimate = 10;
    const size_t docs = 10;
    auto shard = std::make_shared<core::Shard>(loadFactor, estimate, docs);

    shard->insert("apple", "first.txt", {docs}, 145);
    shard->insert("apple", "second.txt", {docs}, 34);
    shard->insert("apple", "first.txt", {docs}, 99);
    shard->insert("orange", "second.txt", {docs}, 55);
    shard->insert("apple", "first.txt", {docs}, 98);
    shard->insert("raspberry", "first.txt", {docs},88);

    EXPECT_TRUE(shard->exists("apple"));
    EXPECT_TRUE(shard->isExpandable());

    bool exists;
    auto record = shard->search("apple", exists);

    EXPECT_TRUE(exists);
    EXPECT_EQ(record->size(), 4);

    EXPECT_EQ(shard->tokenCount(), 3);

    shard->erase("orange");

    bool orangeExists;
    auto nullRecord = shard->search("orange", orangeExists);

    EXPECT_FALSE(orangeExists);
    EXPECT_FALSE(shard->exists("orange"));
    EXPECT_EQ(nullRecord->serialize(), Json(nullptr));
    EXPECT_EQ(shard->tokenCount(), 2);
}