cmake_minimum_required(VERSION 3.16)
project(serpent C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen, sscanf in the tests
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(serpent STATIC src/serpent.c)

add_executable(serpent_demo src/serpent_demo.c)
target_link_libraries(serpent_demo PRIVATE serpent)
add_executable(test_vectors tests/test_vectors.c)
target_link_libraries(test_vectors PRIVATE serpent)
foreach(p round_trip_only word_byte_order ecb_patterns)
  add_executable(pitfall_${p} pitfalls/${p}.c)
  target_link_libraries(pitfall_${p} PRIVATE serpent)
endforeach()

enable_testing()
set(VECTORS ${CMAKE_CURRENT_SOURCE_DIR}/tests/vectors/serpent_nessie.txt)
function(expect_output name expected prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}"
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
expect_output(demo_output serpent_demo.txt $<TARGET_FILE:serpent_demo>)
expect_output(vectors test_vectors.txt $<TARGET_FILE:test_vectors> ${VECTORS})
expect_output(pitfall_round_trip_only pitfall_round_trip_only.txt
              $<TARGET_FILE:pitfall_round_trip_only> ${VECTORS})
expect_output(pitfall_word_byte_order pitfall_word_byte_order.txt
              $<TARGET_FILE:pitfall_word_byte_order> ${VECTORS})
expect_output(pitfall_ecb_patterns pitfall_ecb_patterns.txt $<TARGET_FILE:pitfall_ecb_patterns>)

# An independent implementation: libgcrypt's Serpent, when it is installed.
find_library(GCRYPT_LIBRARY gcrypt)
find_path(GCRYPT_INCLUDE_DIR gcrypt.h)
if(GCRYPT_LIBRARY AND GCRYPT_INCLUDE_DIR)
  add_executable(test_against_libgcrypt tests/test_against_libgcrypt.c)
  target_include_directories(test_against_libgcrypt PRIVATE ${GCRYPT_INCLUDE_DIR})
  target_link_libraries(test_against_libgcrypt PRIVATE serpent ${GCRYPT_LIBRARY})
  add_test(NAME against_libgcrypt COMMAND test_against_libgcrypt)
endif()

if(NOT WIN32)       # clock_gettime is POSIX; the benchmark is built, not run
  add_executable(serpent_speed bench/serpent_speed.c)
  target_link_libraries(serpent_speed PRIVATE serpent)
  if(GCRYPT_LIBRARY AND GCRYPT_INCLUDE_DIR)
    target_compile_definitions(serpent_speed PRIVATE HAVE_LIBGCRYPT)
    target_include_directories(serpent_speed PRIVATE ${GCRYPT_INCLUDE_DIR})
    target_link_libraries(serpent_speed PRIVATE ${GCRYPT_LIBRARY})
  endif()
endif()
