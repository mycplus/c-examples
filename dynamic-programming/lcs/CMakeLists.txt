cmake_minimum_required(VERSION 3.16)
project(lcs C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(lcs STATIC src/lcs.c)

add_executable(lcs_demo src/lcs_demo.c)          # the article's listing
add_executable(lcs_diff src/lcs_diff.c)          # lcs_diff OLD NEW
add_executable(test_lcs tests/test_lcs.c)
foreach(t lcs_demo lcs_diff test_lcs)
  target_link_libraries(${t} PRIVATE lcs)
endforeach()
add_executable(pitfall_naive_recursion pitfalls/naive_recursion.c)
if(NOT WIN32)
  # Frees a string literal on purpose; the Linux workflow checks that it
  # crashes. Not built on Windows, where the result depends on the heap.
  add_executable(pitfall_free_literal pitfalls/free_literal.c)
  add_executable(lcs_speed bench/lcs_speed.c)    # built, not run by ctest
  target_link_libraries(lcs_speed PRIVATE lcs)
endif()

enable_testing()
add_test(NAME unit_and_brute_force_tests COMMAND test_lcs)

function(expect_output name expected rc prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}" -DEXPECT_RC=${rc}
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
set(D ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)
expect_output(demo_output lcs_demo.txt 0 $<TARGET_FILE:lcs_demo>)
expect_output(diff_output lcs_diff.txt 1 $<TARGET_FILE:lcs_diff> "${D}/before.c,${D}/after.c")
expect_output(diff_same_file lcs_diff_same.txt 0 $<TARGET_FILE:lcs_diff> "${D}/before.c,${D}/before.c")
expect_output(naive_recursion_output naive_recursion.txt 0 $<TARGET_FILE:pitfall_naive_recursion>)
add_test(NAME diff_rejects_missing_file COMMAND lcs_diff no-such-file.txt ${D}/after.c)
set_tests_properties(diff_rejects_missing_file PROPERTIES WILL_FAIL TRUE)
