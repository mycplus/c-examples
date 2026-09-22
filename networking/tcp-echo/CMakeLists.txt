cmake_minimum_required(VERSION 3.16)
project(tcp_echo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(net STATIC src/net.c)
target_include_directories(net PUBLIC include)

add_executable(tcp-server src/tcp-server.c)
target_link_libraries(tcp-server PRIVATE net Threads::Threads)

add_executable(tcp-client src/tcp-client.c)
target_link_libraries(tcp-client PRIVATE net)
