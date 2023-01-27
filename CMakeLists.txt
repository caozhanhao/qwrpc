cmake_minimum_required(VERSION 3.8.2)
project(qwrpc)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)
include_directories(thirdparty/libczh/include)

add_executable(qwrpc-server
        examples/server.cpp)
add_executable(qwrpc-client
        examples/client.cpp)

find_package(Threads REQUIRED)
target_link_libraries(qwrpc-server PUBLIC Threads::Threads)
target_link_libraries(qwrpc-client PUBLIC Threads::Threads)
