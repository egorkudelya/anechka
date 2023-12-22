cmake_minimum_required(VERSION 3.22.1)
project(anechka)

set(CMAKE_CXX_STANDARD 17)

find_package(PythonInterp)
find_package(Python)

execute_process(COMMAND python3 src/network/generator/main.py
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(vendor/hash)
include_directories(src/hash)
include_directories(src/utility)

add_library(timer STATIC
        src/utility/timer.h
        src/utility/timer.cpp
)

add_library(mmap STATIC
        src/mmap/mmap.h
        src/mmap/mmap.cpp
)

add_library(engine STATIC
        src/engine/engine.h
        src/engine/engine.cpp
        src/engine/shard.h
        src/engine/shard.cpp
)

add_library(thread_pool STATIC
        src/thread_pool/primitives/task.h
        src/thread_pool/primitives/waitable_future.h
        src/thread_pool/primitives/waitable_future.cpp
        src/thread_pool/pool/thread_pool.h
        src/thread_pool/pool/thread_pool.cpp
        src/thread_pool/dispatch/dispatch.cpp
        src/thread_pool/dispatch/dispatch.h
        src/thread_pool/primitives/callback.cpp
        src/thread_pool/primitives/callback.h
)

target_link_libraries(engine PRIVATE mmap thread_pool timer)

add_library(network_common STATIC
        src/network/abstract/message.h
        src/network/abstract/message.cpp
        src/network/abstract/cstub.h
        src/network/abstract/cstub.cpp
        src/network/abstract/sstub.h
        src/network/abstract/sstub.cpp

        src/network/common/proto_utils.h
        src/network/common/proto_utils.cpp

        src/network/gen/gen_messages.h
        src/network/gen/gen_messages.cpp
)

add_library(servl STATIC
        src/network/gen/gen_server_stub.h
        src/network/gen/gen_server_stub.cpp
        src/server/server.h
        src/server/server.cpp
)

add_library(cl STATIC
        src/network/gen/gen_server_stub.h
        src/network/gen/gen_server_stub.cpp
        src/client/client.h
        src/client/client.cpp
)

target_link_libraries(servl PRIVATE engine network_common)
target_link_libraries(cl PRIVATE network_common)

add_executable(server server.cpp)
add_executable(client client.cpp)

target_link_libraries(server PRIVATE servl)
target_link_libraries(client PRIVATE cl)

# tests
add_subdirectory(vendor/googletest)

set(TEST_SRC
    test/umap_test.cpp
    test/mmap_test.cpp
    test/shard_test.cpp
    test/search_engine_test.cpp
    test/anechka_test.cpp
    test/valgrind.cpp
    test/performance_test.cpp
)

set(test_libs gtest_main servl cl)
set(i 0)

function(add_test_file TEST_NAME TEST_FILE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE ${test_libs})
endfunction()

foreach(file ${TEST_SRC})
    add_test_file("a_test${i}" ${file})
    math(EXPR i "${i} + 1")
endforeach()
