cmake_minimum_required(VERSION 3.16)
project(arrays_in_c C)

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

set(PROGRAMS declare layout grades functions copy_compare playlist seating row_major)
foreach(prog ${PROGRAMS})
  add_executable(${prog} src/${prog}.c)
endforeach()
add_executable(test_arrays tests/test_arrays.c)

enable_testing()
add_test(NAME unit_arrays COMMAND test_arrays)
# The article's printed outputs, checked byte for byte
foreach(prog ${PROGRAMS})
  add_test(NAME output_${prog}
           COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:${prog}>
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${prog}.txt
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_test(NAME pitfalls COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/pitfalls_test.sh ${CMAKE_C_COMPILER})
endif()
