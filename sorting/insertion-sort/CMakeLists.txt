cmake_minimum_required(VERSION 3.16)
project(insertion_sort C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)

add_executable(insertion_example src/insertion_example.c src/insertion_sort.c)
add_executable(insertion_demo src/insertion_demo.c src/insertion_sort.c)
add_executable(insertion_count src/insertion_count.c)   # #includes src/insertion_sort.c to count comparisons
if(NOT WIN32)        # clock_gettime is POSIX
  add_executable(insertion_time src/insertion_time.c src/insertion_sort.c)
endif()

add_executable(test_insertion_sort tests/test_insertion_sort.c src/insertion_sort.c)

enable_testing()
add_test(NAME test_insertion_sort COMMAND test_insertion_sort)
add_test(NAME insertion_example_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:insertion_example>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/insertion_example.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME insertion_demo_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:insertion_demo>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/insertion_demo.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME insertion_count_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:insertion_count>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/insertion_count.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)

# pitfalls/stability.c is a correct program whose output the article explains
add_executable(pitfall_stability pitfalls/stability.c)
add_test(NAME pitfall_stability_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:pitfall_stability>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/pitfall_stability.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
