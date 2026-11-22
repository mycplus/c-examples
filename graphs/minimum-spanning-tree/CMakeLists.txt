cmake_minimum_required(VERSION 3.16)
project(minimum_spanning_tree C CXX)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/permissive->)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(mst STATIC src/mst.c)

add_executable(mst_demo src/mst_demo.c)          # the article's worked example
add_executable(test_mst tests/test_mst.c)
add_executable(mst_speed bench/mst_speed.c)      # built, not run by ctest
foreach(t mst_demo test_mst mst_speed)
  target_link_libraries(${t} PRIVATE mst)
endforeach()

add_executable(mst_cpp cpp/mst.cpp)              # the C++17 version

# The pitfalls are wrong on purpose, but compile cleanly.
add_executable(pitfall_no_cycle_check pitfalls/no_cycle_check.c)
add_executable(pitfall_subtract_compare pitfalls/subtract_compare.c)
add_executable(pitfall_naive_union_find pitfalls/naive_union_find.c)

enable_testing()
add_test(NAME unit_and_random_tests COMMAND test_mst)
add_test(NAME cpp_random_tests COMMAND mst_cpp --test)

function(expect_output name expected prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}"
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
expect_output(demo_output mst_demo.txt $<TARGET_FILE:mst_demo>)
expect_output(cpp_output mst_cpp.txt $<TARGET_FILE:mst_cpp>)
expect_output(no_cycle_check_output no_cycle_check.txt $<TARGET_FILE:pitfall_no_cycle_check>)
expect_output(naive_union_find_output naive_union_find.txt $<TARGET_FILE:pitfall_naive_union_find>)
# The subtracting comparator overflows, which is undefined behaviour: its
# output depends on the C library's qsort, and UBSan stops it on purpose.
# Its output is only checked with glibc and no sanitizer.
if(NOT WIN32 AND NOT CMAKE_C_FLAGS MATCHES "sanitize")
  expect_output(subtract_compare_output subtract_compare.txt $<TARGET_FILE:pitfall_subtract_compare>)
endif()
