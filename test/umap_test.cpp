#include <gtest/gtest.h>
#include "xxh64_hasher.h"
#include "../src/hash/umap.h"

TEST(SUMapTest, Basic)
{
    core::SUMap<std::string, std::string, core::Xxh64Hasher> map(10);
    map.insert("Hello", "World");
    map.emplace(std::make_pair("Washington", "USA"));
    map.emplace(std::make_pair("Kyiv", "Ukraine"));
    map.emplace(std::make_pair("Warsaw", "Poland"));
    map.emplace(std::make_pair("London", "UK"));

    EXPECT_TRUE(map.exists("Hello"));

    map.erase("Hello");
    EXPECT_FALSE(map.exists("Hello"));

    EXPECT_EQ(map.size(), 4);
}