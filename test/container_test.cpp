#include <gtest/gtest.h>
#include "xxh64_hasher.h"
#include "../src/hash/umap.h"

TEST(SUMapTest, Basic)
{
    core::SUMap<std::string, std::string, core::Xxh64Hasher> map(10);

    for (auto&&[first, second]: map.iterate())
    {
        ASSERT_FALSE("This should not have happened");
    }

    map.insert("Hello", "World");
    map.emplace(std::make_pair("Washington", "USA"));
    map.emplace(std::make_pair("Kyiv", "Ukraine"));
    map.emplace(std::make_pair("Warsaw", "Poland"));
    map.emplace(std::make_pair("London", "UK"));

    EXPECT_TRUE(map.contains("Hello"));

    map.erase("Hello");
    EXPECT_FALSE(map.contains("Hello"));

    EXPECT_EQ(map.size(), 4);

    std::unordered_map<std::string, std::string> dump;
    for (auto&&[first, second]: map.iterate())
    {
        dump[first] = second;
    }

    for (auto&&[first, second]: dump)
    {
        auto val = map.get(first);
        EXPECT_EQ(val, second);
    }
}

TEST(USetTest, Basic)
{
    core::USet<std::string, core::Xxh64Hasher> set(10);

    for (auto&& val: set.iterate())
    {
        ASSERT_FALSE("This should not have happened");
    }

    set.addIfNotPresent("Hello");
    set.addIfNotPresent("USA");
    set.addIfNotPresent("Ukraine");
    set.addIfNotPresent("Poland");
    set.addIfNotPresent("UK");

    set.erase("Hello");
    EXPECT_FALSE(set.contains("Hello"));

    EXPECT_EQ(set.size(), 4);

    std::vector<std::string> dump;
    for (auto&& val: set.iterate())
    {
        dump.push_back(val);
    }

    for (auto&& val: dump)
    {
        ASSERT_TRUE(set.contains(val));
    }
}