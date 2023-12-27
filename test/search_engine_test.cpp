#include <gtest/gtest.h>
#include "../src/engine/engine.h"

TEST(SearchEngineTest, Basic)
{
    auto engine = std::make_unique<core::SearchEngine>(core::SearchEngineParams{25, 1, 8, 0.75, true});

    engine->indexTxtFile("../test/data/sample.txt");

    EXPECT_EQ(engine->tokenCount(), 10);

    bool found = false;
    auto res = engine->search("nothing", found);

    EXPECT_TRUE(found);

    engine->erase("nothing");
    EXPECT_EQ(engine->search("nothing", found)->serialize(), Json(nullptr));
    EXPECT_FALSE(found);

    EXPECT_EQ(engine->search("idontexist", found)->serialize(), Json(nullptr));
    EXPECT_FALSE(found);

    engine->dump("dump.json");
    auto restoredEngine = std::make_unique<core::SearchEngine>(core::SearchEngineParams{25, 1, 8, 0.75});

    bool stale;
    restoredEngine->restore("dump.json", stale);

    EXPECT_EQ(engine->tokenCount(), restoredEngine->tokenCount());

    bool originalFound = false;
    auto originalRes = engine->search("acquire", originalFound);
    EXPECT_TRUE(originalFound);

    bool restoreFound = false;
    auto restoreRes = restoredEngine->search("acquire", restoreFound);
    EXPECT_TRUE(restoreFound);
    EXPECT_EQ(originalRes->serialize(), restoreRes->serialize());

    // I don't compare contents here because tokens may be in a different order in the dumps
    EXPECT_EQ(engine->serialize().dump().size(), restoredEngine->serialize().dump().size());
}
