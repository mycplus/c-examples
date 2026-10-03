cmake_minimum_required(VERSION 3.16)
project(selection_sort C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)

add_executable(selection_example src/selection_example.c src/selection_sort.c)
add_executable(selection_demo src/selection_demo.c)
add_executable(selection_count src/selection_count.c)   # #includes src/selection_sort.c to count comparisons
if(NOT WIN32)        # clock_gettime is POSIX
  add_executable(selection_time src/selection_time.c src/selection_sort.c)
endif()

add_executable(test_selection_sort tests/test_selection_sort.c src/selection_sort.c)

enable_testing()
add_test(NAME test_selection_sort COMMAND test_selection_sort)
add_test(NAME selection_example_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:selection_example>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/selection_example.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME selection_demo_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:selection_demo>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/selection_demo.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME selection_count_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:selection_count>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/selection_count.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)

# The pitfall programs are deterministic; the tests pin the output the article shows
foreach(p xor_swap exchange_sort stability)
  add_executable(pitfall_${p} pitfalls/${p}.c)
  add_test(NAME pitfall_${p}_output COMMAND ${CMAKE_COMMAND}
    -DPROG=$<TARGET_FILE:pitfall_${p}>
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/pitfall_${p}.txt
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()

if(NOT WIN32)
  add_executable(value_in_register experiments/value_in_register.c)
endif()
