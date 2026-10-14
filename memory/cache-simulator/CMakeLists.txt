cmake_minimum_required(VERSION 3.16)
project(cache_simulator C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen, strerror
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)

add_library(cachesim_core STATIC src/cache.c src/trace.c)
add_executable(cachesim src/cachesim.c)
add_executable(belady src/belady.c)
add_executable(matrix_trace src/matrix_trace.c)
add_executable(matrix_sum src/matrix_sum.c)
add_executable(test_cache tests/test_cache.c)
target_link_libraries(cachesim cachesim_core)
target_link_libraries(belady cachesim_core)
target_link_libraries(test_cache cachesim_core)

enable_testing()
add_test(NAME test_cache COMMAND test_cache)

function(expect_output name expected prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}"
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()

set(data ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)
expect_output(belady_output belady.txt $<TARGET_FILE:belady>)
expect_output(small_lru small_lru.txt $<TARGET_FILE:cachesim> "128,assoc:2,lru,64,${data}/small.trace")
expect_output(small_fifo small_fifo.txt $<TARGET_FILE:cachesim> "128,assoc:2,fifo,64,${data}/small.trace")
expect_output(lackey_format lackey.txt $<TARGET_FILE:cachesim> "1024,assoc:2,lru,64,${data}/lackey.trace")

# matrix_trace | cachesim, row and column order, 8-way and fully associative.
foreach(order row col)
  foreach(ways assoc:8 assoc)
    string(REPLACE ":" "" tag "${order}_${ways}")
    add_test(NAME matrix_${tag} COMMAND ${CMAKE_COMMAND}
      -DNAME=matrix_${tag} -DGEN=$<TARGET_FILE:matrix_trace> -DGEN_ARGS=${order}
      -DSIM=$<TARGET_FILE:cachesim> "-DSIM_ARGS=32768,${ways},lru,64"
      -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/matrix_${tag}.txt
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/pipe_output.cmake)
  endforeach()
endforeach()
add_test(NAME matrix_col257_assoc8 COMMAND ${CMAKE_COMMAND}
  -DNAME=matrix_col257 -DGEN=$<TARGET_FILE:matrix_trace> "-DGEN_ARGS=col,257"
  -DSIM=$<TARGET_FILE:cachesim> "-DSIM_ARGS=32768,assoc:8,lru,64"
  -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/matrix_col257_assoc8.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/pipe_output.cmake)

# Bad arguments and bad traces are rejected with a non-zero exit status.
add_test(NAME rejects_no_arguments COMMAND cachesim)
add_test(NAME rejects_size_zero COMMAND cachesim 0 direct lru 64 ${data}/small.trace)
add_test(NAME rejects_letters COMMAND cachesim abc direct lru 64 ${data}/small.trace)
add_test(NAME rejects_odd_block COMMAND cachesim 32768 direct lru 48 ${data}/small.trace)
add_test(NAME rejects_too_many_ways COMMAND cachesim 128 assoc:4 lru 64 ${data}/small.trace)
add_test(NAME rejects_bad_assoc COMMAND cachesim 32768 assoc:x lru 64 ${data}/small.trace)
add_test(NAME rejects_bad_policy COMMAND cachesim 32768 direct random 64 ${data}/small.trace)
add_test(NAME rejects_missing_file COMMAND cachesim 32768 direct lru 64 no-such-file.trace)
add_test(NAME rejects_bad_trace COMMAND cachesim 32768 direct lru 64 ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
set_tests_properties(rejects_no_arguments rejects_size_zero rejects_letters rejects_odd_block
                     rejects_too_many_ways rejects_bad_assoc rejects_bad_policy
                     rejects_missing_file rejects_bad_trace PROPERTIES WILL_FAIL TRUE)
