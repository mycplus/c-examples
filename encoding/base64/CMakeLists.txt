cmake_minimum_required(VERSION 3.16)
project(base64 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(base64 STATIC src/base64.c)
target_include_directories(base64 PUBLIC include)

add_executable(base64-demo src/base64-demo.c)
target_link_libraries(base64-demo PRIVATE base64)

enable_testing()
add_executable(test_base64 tests/test_base64.c)
target_link_libraries(test_base64 PRIVATE base64)
add_test(NAME base64_tests COMMAND test_base64)
