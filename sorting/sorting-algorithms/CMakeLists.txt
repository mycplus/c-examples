cmake_minimum_required(VERSION 3.16)
project(sorting_algorithms C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)

# sort_count.c and stability.c #include sorting.c so they can redefine SORT_LESS
add_executable(sort_count src/sort_count.c)
add_executable(stability  src/stability.c)
add_executable(test_sorting tests/test_sorting.c src/sorting.c)

if(NOT WIN32)          # clock_gettime is POSIX
  add_executable(sort_time src/sort_time.c src/sorting.c)
endif()

enable_testing()
add_test(NAME correctness COMMAND test_sorting)
add_test(NAME comparison_counts COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:sort_count>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/sort_count.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
# The same counting program with the merge sort skip check turned off: its
# merge sort row matches the qsort() row, as the article reports.
add_executable(sort_count_noskip src/sort_count.c)
target_compile_definitions(sort_count_noskip PRIVATE MERGE_SKIP_SORTED=0)
add_test(NAME comparison_counts_noskip COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:sort_count_noskip>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/sort_count_noskip.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME stability COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:stability>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/stability.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
