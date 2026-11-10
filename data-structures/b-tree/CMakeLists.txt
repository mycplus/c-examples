cmake_minimum_required(VERSION 3.16)
project(b_tree C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(btree STATIC src/btree.c)

add_executable(btree_demo src/btree_demo.c)     # the article's listing
add_executable(btree_shape src/btree_shape.c)   # height and fill for 1,000,000 keys
add_executable(test_btree tests/test_btree.c)
foreach(t btree_demo btree_shape test_btree)
  target_link_libraries(${t} PRIVATE btree)
endforeach()
if(NOT MSVC)
  target_link_libraries(btree_shape PRIVATE m)
endif()
if(NOT WIN32)                                   # clock_gettime; built, not run by ctest
  add_executable(btree_speed bench/btree_speed.c)
  target_link_libraries(btree_speed PRIVATE btree)
endif()

enable_testing()
add_test(NAME unit_and_model_tests COMMAND test_btree)

function(expect_output name expected prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}"
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
expect_output(demo_output btree_demo.txt $<TARGET_FILE:btree_demo>)
expect_output(shape_output btree_shape.txt $<TARGET_FILE:btree_shape>)
