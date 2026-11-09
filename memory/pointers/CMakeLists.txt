cmake_minimum_required(VERSION 3.16)
project(pointers_c C)

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

set(PROGRAMS basics sizes arrays functions strings const_pointers
             dynamic_array linked_list callbacks generic_swap)
foreach(prog ${PROGRAMS} address_demo double_pointer)
  add_executable(${prog} src/${prog}.c)
endforeach()

foreach(t pointer_arith functions text dynamic_array linked_list generic)
  add_executable(test_${t} tests/test_${t}.c)
endforeach()

enable_testing()
foreach(t pointer_arith functions text dynamic_array linked_list generic)
  add_test(NAME unit_${t} COMMAND test_${t})
endforeach()

# The article's printed outputs, checked byte for byte
foreach(prog ${PROGRAMS})
  add_test(NAME output_${prog}
           COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:${prog}>
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${prog}.txt
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()
add_test(NAME output_double_pointer
         COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:double_pointer>
                 "-DARGS=alpha beta"
                 -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/double_pointer.txt
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)

# Addresses change on every run, so only the shape of the output is checked.
add_test(NAME shape_address_demo COMMAND address_demo)
set_tests_properties(shape_address_demo PROPERTIES
  PASS_REGULAR_EXPRESSION "score: value 70, address .*p: +value .*sizeof\\(p\\) = [48]")
