cmake_minimum_required(VERSION 3.20)
project(defensive_programming LANGUAGES C)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  set(WARNINGS /W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS) # fopen and strcpy are used on purpose
else()
  set(WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2 -Werror)
endif()

add_library(defensive STATIC src/defensive.c)
target_include_directories(defensive PUBLIC src)
target_compile_options(defensive PRIVATE ${WARNINGS})

add_executable(people src/people.c)
add_executable(test_defensive tests/test_defensive.c)
add_executable(pitfall_atoi pitfalls/atoi_vs_strtol.c)
foreach(t people test_defensive pitfall_atoi)
  target_link_libraries(${t} PRIVATE defensive)
  target_compile_options(${t} PRIVATE ${WARNINGS})
endforeach()

# The same assert built twice: as a debug build and with NDEBUG.
add_executable(pitfall_assert pitfalls/assert_side_effect.c)
add_executable(pitfall_assert_ndebug pitfalls/assert_side_effect.c)
target_compile_options(pitfall_assert PRIVATE -UNDEBUG) # even in a Release build
target_compile_definitions(pitfall_assert_ndebug PRIVATE NDEBUG)

# Programs with deliberate memory bugs, for the sanitizer checks in run_all.sh.
# They are built with warnings but not as errors: GCC already spots one of them.
foreach(p off_by_one signed_overflow use_after_free leak_on_error)
  add_executable(pitfall_${p} pitfalls/${p}.c)
endforeach()

enable_testing()
set(TEMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/tmp)
file(MAKE_DIRECTORY ${TEMP_DIR})
add_test(NAME unit_tests COMMAND test_defensive ${CMAKE_CURRENT_SOURCE_DIR}/data ${TEMP_DIR})

function(output_test name prog expected)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=$<TARGET_FILE:${prog}> -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    "-DARGS=${ARGN}" -DWORKDIR=${CMAKE_CURRENT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
output_test(people_output people people.txt data/people.txt)
output_test(atoi_output pitfall_atoi atoi.txt)
output_test(assert_debug pitfall_assert assert-debug.txt)
output_test(assert_ndebug pitfall_assert_ndebug assert-ndebug.txt)

# Code that must not compile.
# expect_build_failure.cmake compiles SOURCE and passes only if the compiler
# fails with a message matching EXPECT.
function(compile_fail_test name source expect)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DCC=${CMAKE_C_COMPILER} -DMSVC=${MSVC} -DSTD=${CMAKE_C_STANDARD}
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/${source} -DOBJ=${TEMP_DIR}/${name}.obj
    "-DEXPECT=${expect}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_build_failure.cmake)
endfunction()
compile_fail_test(static_assert_fails compile_fail/header_size.c "header must be 7 bytes on disk")
if(NOT MSVC)
  # warn_unused_result is a GCC and Clang attribute.
  compile_fail_test(ignored_status_fails compile_fail/ignored_status.c "unused.result")
endif()
