cmake_minimum_required(VERSION 3.16)
project(linear_search C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen, strtok, scanf, strcpy
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(linear_search STATIC src/linear_search.c)

add_executable(linear_search_demo src/linear_search_demo.c)   # the article's examples
add_executable(search_numbers src/search_numbers.c)           # search_numbers FILE KEY...
add_executable(test_linear_search tests/test_linear_search.c)
add_executable(comparisons bench/comparisons.c)               # deterministic counts
foreach(t linear_search_demo search_numbers test_linear_search comparisons)
  target_link_libraries(${t} PRIVATE linear_search)
endforeach()

# The pitfalls are wrong on purpose.
add_executable(pitfall_off_by_one pitfalls/off_by_one.c)
add_executable(pitfall_unchecked_scanf pitfalls/unchecked_scanf.c)
add_executable(pitfall_string_equals pitfalls/string_equals.c)
# Their warnings are part of the lesson (GCC notices two of them when
# optimizing), so they are shown but do not stop the build.
foreach(t pitfall_off_by_one pitfall_unchecked_scanf pitfall_string_equals)
  target_compile_options(${t} PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/WX-,-Wno-error>)
endforeach()
if(NOT WIN32)
  add_executable(test_lfind tests/test_lfind.c)              # POSIX lfind()
  target_link_libraries(test_lfind PRIVATE linear_search)
  add_executable(search_speed bench/search_speed.c)          # built, not run by ctest
  target_link_libraries(search_speed PRIVATE linear_search)
endif()

enable_testing()
add_test(NAME unit_and_random_tests COMMAND test_linear_search)
if(NOT WIN32)
  add_test(NAME agrees_with_lfind COMMAND test_lfind)
endif()

function(expect_output name expected rc prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}" -DEXPECT_RC=${rc}
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
set(D ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)
expect_output(demo_output linear_search_demo.txt 0 $<TARGET_FILE:linear_search_demo>)
expect_output(comparisons_output comparisons.txt 0 $<TARGET_FILE:comparisons>)
expect_output(search_numbers_output search_numbers.txt 1 $<TARGET_FILE:search_numbers>
              "${D}/numbers.txt,73,0,50")
expect_output(off_by_one_output off_by_one.txt 0 $<TARGET_FILE:pitfall_off_by_one>)
expect_output(string_equals_output string_equals.txt 0 $<TARGET_FILE:pitfall_string_equals>)
add_test(NAME unchecked_scanf_output COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:pitfall_unchecked_scanf> -DINPUT=${D}/scanf_input.txt
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/unchecked_scanf.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
add_test(NAME search_numbers_rejects_bad_input COMMAND search_numbers ${D}/bad.txt 4)
set_tests_properties(search_numbers_rejects_bad_input PROPERTIES WILL_FAIL TRUE)
