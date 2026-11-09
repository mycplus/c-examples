cmake_minimum_required(VERSION 3.16)
project(binary_tree_c C)

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

add_executable(bst       src/bst.c)
add_executable(bst_shape src/bst_shape.c)
add_executable(deep_tree src/deep_tree.c)
add_executable(test_bst  tests/test_bst.c)

enable_testing()
add_test(NAME bst_unit COMMAND test_bst)
# The article's printed output, checked byte for byte
add_test(NAME output_bst
         COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:bst>
                 -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/bst.txt
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
# A chain of a million nodes: the iterative height does not recurse
add_test(NAME deep_tree_iterative COMMAND deep_tree 1000000 iterative)
set_tests_properties(deep_tree_iterative PROPERTIES
                     PASS_REGULAR_EXPRESSION "iterative height = 999999")
