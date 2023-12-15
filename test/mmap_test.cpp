#include <gtest/gtest.h>
#include "../src/mmap/mmap.h"

TEST(MMapTest, FileException)
{
    std::string wrongPath{"/idontexist.txt"};
    EXPECT_THROW({
        try
        {
            const MMapASCII mmap(wrongPath);
        }
        catch (const std::invalid_argument& err)
        {
            EXPECT_STREQ("Invalid filepath", err.what());
            throw;
        }
    }, std::invalid_argument);
}

TEST(MMapTest, BasicOps)
{
    std::string path{"../test/data/sample.txt"};
    const MMapASCII mmap(path);

    EXPECT_EQ(mmap.size(), 79);

    std::string content(mmap.begin(), mmap.end());

    EXPECT_EQ(content, "The more I read, the more I acquire, the more certain I am that I know nothing.");
    EXPECT_EQ(content.size(), 79);
}