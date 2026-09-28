cmake_minimum_required(VERSION 3.16)
project(stack_c C)

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

add_executable(array_stack  src/array_stack.c)
add_executable(linked_stack src/linked_stack.c)
add_executable(growth       src/growth.c)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")   # mallinfo2() is glibc-specific
  add_executable(memory     src/memory.c)
endif()

add_executable(test_array_stack  tests/test_array_stack.c)
add_executable(test_linked_stack tests/test_linked_stack.c)

enable_testing()
add_test(NAME array_stack_unit  COMMAND test_array_stack)
add_test(NAME linked_stack_unit COMMAND test_linked_stack)
# The article's printed outputs, checked byte for byte
foreach(prog array_stack linked_stack growth)
  add_test(NAME output_${prog}
           COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:${prog}>
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${prog}.txt
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()
