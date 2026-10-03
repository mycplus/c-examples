cmake_minimum_required(VERSION 3.16)
project(heap_sort C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)

add_executable(heap_example src/heap_example.c src/heap_sort.c)
add_executable(heap_demo src/heap_demo.c src/heap_sort.c)
add_executable(heap_count src/heap_count.c)   # #includes src/heap_sort.c to count comparisons
if(NOT WIN32)        # clock_gettime is POSIX
  add_executable(heap_time src/heap_time.c src/heap_sort.c)
endif()

add_executable(test_heap_sort tests/test_heap_sort.c src/heap_sort.c)

enable_testing()
add_test(NAME test_heap_sort COMMAND test_heap_sort)
add_test(NAME heap_example_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:heap_example>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/heap_example.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME heap_demo_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:heap_demo>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/heap_demo.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME heap_count_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:heap_count>
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/heap_count.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)

target_link_libraries(heap_count PRIVATE $<$<NOT:$<C_COMPILER_ID:MSVC>>:m>)   # log2()

# Deterministic pitfall programs: the tests pin the output the article shows.
# child_bound.c reads past the array (undefined behavior), so its output is
# not pinned; the workflow checks that AddressSanitizer reports it instead.
foreach(p one_based one_based_search)
  add_executable(pitfall_${p} pitfalls/${p}.c)
  add_test(NAME pitfall_${p}_output COMMAND ${CMAKE_COMMAND}
    -DPROG=$<TARGET_FILE:pitfall_${p}>
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/pitfall_${p}.txt
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()
