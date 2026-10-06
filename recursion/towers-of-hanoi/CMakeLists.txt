cmake_minimum_required(VERSION 3.16)
project(towers_of_hanoi C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)

add_executable(towers_of_hanoi src/towers_of_hanoi.c)     # the article's listings
add_executable(towers_of_hanoi_iterative src/towers_of_hanoi_iterative.c)
add_executable(hanoi_cli src/hanoi_cli.c src/hanoi.c)
add_executable(test_hanoi tests/test_hanoi.c src/hanoi.c)
if(NOT WIN32)        # clock_gettime is POSIX
  add_executable(hanoi_time src/hanoi_time.c src/hanoi.c)
endif()
add_executable(pitfall_copied_arguments pitfalls/copied_arguments.c)

enable_testing()
add_test(NAME test_hanoi COMMAND test_hanoi)

function(expect_output name expected prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}"
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()

expect_output(listing_output towers_of_hanoi.txt $<TARGET_FILE:towers_of_hanoi>)
expect_output(iterative_listing_output towers_of_hanoi.txt $<TARGET_FILE:towers_of_hanoi_iterative>)
# The command-line program prints the same seven moves both ways.
expect_output(cli_recursive_output towers_of_hanoi.txt $<TARGET_FILE:hanoi_cli> 3)
expect_output(cli_iterative_output towers_of_hanoi.txt $<TARGET_FILE:hanoi_cli> "3,--iterative")
expect_output(pitfall_copied_arguments_output pitfall_copied_arguments.txt
              $<TARGET_FILE:pitfall_copied_arguments>)

# Bad arguments are rejected with a non-zero exit status.
set(bad_args abc -1 21 3x 2.5)
foreach(arg IN LISTS bad_args)
  add_test(NAME cli_rejects_${arg} COMMAND hanoi_cli ${arg})
  set_tests_properties(cli_rejects_${arg} PROPERTIES WILL_FAIL TRUE)
endforeach()
add_test(NAME cli_rejects_empty COMMAND hanoi_cli "")
add_test(NAME cli_rejects_no_argument COMMAND hanoi_cli)
add_test(NAME cli_rejects_bad_flag COMMAND hanoi_cli 3 --fast)
add_test(NAME cli_rejects_leading_space COMMAND hanoi_cli " 3")
add_test(NAME cli_rejects_plus_sign COMMAND hanoi_cli "+3")
set_tests_properties(cli_rejects_empty cli_rejects_no_argument cli_rejects_bad_flag
                     cli_rejects_leading_space cli_rejects_plus_sign
                     PROPERTIES WILL_FAIL TRUE)
