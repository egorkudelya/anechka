#include "../src/client/client.h"
#include "../src/server/server.h"
#include "timer.h"
#include <gtest/gtest.h>
#include <thread>


TEST(Anechka, Perfomance)
{
    const std::filesystem::path path = "../vault";
    ASSERT_TRUE(std::filesystem::exists(path));

    auto anechkaPtr = std::make_unique<anechka::Anechka>("../test/config/config.json");
    anechkaPtr->run();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::thread clientThread = std::thread([&path] {
        auto clientPtr = std::make_unique<net::ClientStub>();
        utils::Timer dirIndexingTimer{};
        clientPtr->RequestRecursiveDirIndexing(path);
        size_t indexInterval = dirIndexingTimer.getInterval();
        std::cerr << "Indexing time: " << indexInterval << "ms" << std::endl;

        utils::Timer contextSearchTimer{};
        clientPtr->RequestTokenSearchWithContext("apple");

        size_t searchInterval = contextSearchTimer.getInterval();
        std::cerr << "Context search time: " << searchInterval << "ms" << std::endl;

        ASSERT_TRUE(searchInterval < 20);
        ASSERT_TRUE(indexInterval < 15000);
    });

    if (clientThread.joinable())
    {
        clientThread.join();
    }

    anechkaPtr->shutDown();
}
