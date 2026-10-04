cmake_minimum_required(VERSION 3.16)
project(merge_sort C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)

add_executable(merge_example src/merge_example.c src/merge_sort.c)
add_executable(merge_demo src/merge_demo.c)
add_executable(merge_count src/merge_count.c)   # #includes src/merge_sort.c to count comparisons
if(NOT WIN32)        # clock_gettime is POSIX
  add_executable(merge_time src/merge_time.c src/merge_sort.c)
endif()

add_executable(test_merge_sort tests/test_merge_sort.c src/merge_sort.c)

enable_testing()
add_test(NAME test_merge_sort COMMAND test_merge_sort)
add_test(NAME merge_example_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:merge_example>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/merge_example.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME merge_demo_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:merge_demo>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/merge_demo.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME merge_count_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:merge_count>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/merge_count.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)

# The same counting program with the skip check turned off
add_executable(merge_count_noskip src/merge_count.c)
target_compile_definitions(merge_count_noskip PRIVATE MERGE_SKIP_SORTED=0)
# MSVC 19.44 (Visual Studio 2022) reports C4127, "conditional expression is
# constant", for `if (MERGE_SKIP_SORTED && ...)` once the macro is 0, and /WX
# turns it into an error. The constant is the point of this build, so that one
# warning is switched off for this target only.
if(MSVC)
  target_compile_options(merge_count_noskip PRIVATE /wd4127)
endif()
add_test(NAME merge_count_noskip_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:merge_count_noskip>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/merge_count_noskip.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)

add_executable(pitfall_ties_right pitfalls/ties_right.c)
add_test(NAME pitfall_ties_right_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:pitfall_ties_right>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/pitfall_ties_right.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
