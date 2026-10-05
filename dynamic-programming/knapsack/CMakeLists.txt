cmake_minimum_required(VERSION 3.16)
project(knapsack C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_executable(knapsack_01 src/knapsack_01.c)
add_executable(knapsack_fractional src/knapsack_fractional.c)
add_executable(test_knapsack tests/test_knapsack.c)   # #includes src/knapsack_01.c

enable_testing()
add_test(NAME test_knapsack COMMAND test_knapsack)
foreach(p knapsack_01 knapsack_fractional)
  add_test(NAME ${p}_output COMMAND ${CMAKE_COMMAND}
    -DPROG=$<TARGET_FILE:${p}>
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${p}.txt
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()

# Deterministic pitfall: the test pins the output the article shows.
# stack_table.c uses a variable-length array (not supported by MSVC) and
# crashes by design, so it is not built here; the workflow checks that
# AddressSanitizer reports the stack overflow instead.
add_executable(pitfall_upward_and_greedy pitfalls/upward_and_greedy.c)
add_test(NAME pitfall_upward_and_greedy_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:pitfall_upward_and_greedy>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/pitfall_upward_and_greedy.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
