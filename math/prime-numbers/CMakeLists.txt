cmake_minimum_required(VERSION 3.16)
project(prime_numbers_c C)

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

add_executable(primes    src/primes.c)
add_executable(check_prime src/check_prime.c)
add_executable(divisions src/divisions.c)
add_executable(benchmark src/benchmark.c)
add_executable(test_primes tests/test_primes.c)

enable_testing()
add_test(NAME primes_unit COMMAND test_primes)
# The article's printed outputs, checked byte for byte
foreach(prog primes divisions)
  add_test(NAME output_${prog}
           COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:${prog}>
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${prog}.txt
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()
# The benchmark exits non-zero if the two methods disagree; a small limit keeps it quick
add_test(NAME benchmark_agrees COMMAND benchmark 1000000)
