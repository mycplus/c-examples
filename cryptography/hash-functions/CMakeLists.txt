cmake_minimum_required(VERSION 3.16)
project(hash_functions C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen and sscanf in the tests
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(digest STATIC src/digest.c)

add_executable(digest_demo src/digest_demo.c)
target_link_libraries(digest_demo PRIVATE digest)
add_executable(collisions src/collisions.c)
target_link_libraries(collisions PRIVATE digest)
add_executable(length_extension src/length_extension.c)
target_link_libraries(length_extension PRIVATE digest)
add_executable(test_vectors tests/test_vectors.c)
target_link_libraries(test_vectors PRIVATE digest)
foreach(p hex_sign_extension strlen_on_binary trailing_newline)
  add_executable(pitfall_${p} pitfalls/${p}.c)
  target_link_libraries(pitfall_${p} PRIVATE digest)
endforeach()

enable_testing()
set(VECTORS ${CMAKE_CURRENT_SOURCE_DIR}/tests/vectors)
function(expect_output name expected prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}"
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
expect_output(demo_output digest_demo.txt $<TARGET_FILE:digest_demo>)
expect_output(vectors test_vectors.txt $<TARGET_FILE:test_vectors> ${VECTORS})
expect_output(collisions collisions.txt $<TARGET_FILE:collisions>
              ${VECTORS}/sha1_collision_1.bin ${VECTORS}/sha1_collision_2.bin)
expect_output(length_extension length_extension.txt $<TARGET_FILE:length_extension>)
foreach(p hex_sign_extension strlen_on_binary trailing_newline)
  expect_output(pitfall_${p} pitfall_${p}.txt $<TARGET_FILE:pitfall_${p}>)
endforeach()

# An independent implementation: OpenSSL, when it is installed.
find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
  add_executable(test_against_openssl tests/test_against_openssl.c)
  target_link_libraries(test_against_openssl PRIVATE digest OpenSSL::Crypto)
  add_test(NAME against_openssl COMMAND test_against_openssl)
endif()

if(NOT WIN32)       # clock_gettime is POSIX; the benchmark is built, not run
  add_executable(digest_speed bench/digest_speed.c)
  target_link_libraries(digest_speed PRIVATE digest)
  if(OPENSSL_FOUND)
    target_compile_definitions(digest_speed PRIVATE HAVE_OPENSSL)
    target_link_libraries(digest_speed PRIVATE OpenSSL::Crypto)
  endif()
endif()
