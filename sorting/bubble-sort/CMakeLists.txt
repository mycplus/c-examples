cmake_minimum_required(VERSION 3.16)
project(bubble_sort_c C)

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_executable(bubble_sort  src/bubble_sort.c)
add_executable(bubble_trace src/bubble_trace.c)
add_executable(bubble_count src/bubble_count.c)
if(NOT WIN32)                      # clock_gettime() is POSIX
  add_executable(bubble_time src/bubble_time.c)
endif()

add_executable(test_bubble_sort tests/test_bubble_sort.c)
add_executable(test_flag        tests/test_flag.c)

enable_testing()
add_test(NAME differential_vs_qsort COMMAND test_bubble_sort)
add_test(NAME flag_matches_boundary COMMAND test_flag)
# The article's printed outputs, checked byte for byte
foreach(prog bubble_sort bubble_trace bubble_count)
  add_test(NAME output_${prog}
           COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:${prog}>
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${prog}.txt
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()
