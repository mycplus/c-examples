cmake_minimum_required(VERSION 3.16)
project(array_operations C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(array_ops STATIC src/array_ops.c)
target_include_directories(array_ops PUBLIC include)

add_executable(array-demo src/array-demo.c)
target_link_libraries(array-demo PRIVATE array_ops)

enable_testing()
add_executable(test_array_ops tests/test_array_ops.c)
target_link_libraries(test_array_ops PRIVATE array_ops)
add_test(NAME array_ops_tests COMMAND test_array_ops)
