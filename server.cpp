#include "../server/server.h"

void shouldQuit()
{
    std::mutex mtx;
    std::condition_variable cv;
    bool done{false};

    std::thread servThread([&]() {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << "Press Enter to quit." << std::endl;
        std::cin.get();
        std::cout << "Shutting down..." << std::endl;
        done = true;
        cv.notify_one();
    });

    std::unique_lock lock(mtx);
    cv.wait(lock, [&]{
        return done;
    });
    if (servThread.joinable())
    {
        servThread.join();
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Error: the program expects the path to config.json" << std::endl;
        return -1;
    }

    const std::string config = argv[1];

    net::stub::ServerStubPtr server = std::make_unique<anechka::Anechka>(config);
    release_assert(server->run(), "Server failed to launch");
    shouldQuit();
    server->shutDown();
    return 0;
}