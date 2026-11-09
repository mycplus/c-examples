cmake_minimum_required(VERSION 3.16)
project(serial_chat C)

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_library(frame STATIC src/frame.c)
target_include_directories(frame PUBLIC include)

if(WIN32)
  add_library(serial STATIC src/serial_win32.c)
else()
  add_library(serial STATIC src/serial_posix.c)
endif()
target_include_directories(serial PUBLIC include)

find_package(Threads REQUIRED)
add_executable(chat src/chat.c)
target_link_libraries(chat PRIVATE serial frame Threads::Threads)

add_executable(frame_demo src/frame_demo.c)
target_link_libraries(frame_demo PRIVATE frame)
add_executable(crc_experiment src/crc_experiment.c)
target_link_libraries(crc_experiment PRIVATE frame)
add_executable(baud_table src/baud_table.c)
add_executable(test_frame tests/test_frame.c)
target_link_libraries(test_frame PRIVATE frame)

enable_testing()
add_test(NAME unit_frame COMMAND test_frame)
foreach(prog frame_demo crc_experiment baud_table)
  add_test(NAME output_${prog}
           COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:${prog}>
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${prog}.txt
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()

# Pseudo-terminal programs and tests: Linux only (CI runs them on Ubuntu).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_library(UTIL_LIB util)
  add_executable(pty_loopback src/pty_loopback.c)
  target_link_libraries(pty_loopback PRIVATE serial frame)
  add_executable(cooked_mode pitfalls/cooked_mode.c)
  if(UTIL_LIB)
    target_link_libraries(pty_loopback PRIVATE ${UTIL_LIB})
    target_link_libraries(cooked_mode PRIVATE ${UTIL_LIB})
  endif()
  foreach(prog pty_loopback cooked_mode)
    add_test(NAME output_${prog}
             COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:${prog}>
                     -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${prog}.txt
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
  endforeach()

  find_program(SOCAT socat)
  if(SOCAT)
    add_test(NAME chat_two_terminals
             COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/chat_test.sh $<TARGET_FILE:chat>)
  endif()

  # DOS-era code must still fail to build, with the error the article quotes.
  add_test(NAME dos_code_does_not_build
           COMMAND sh -c "LC_ALL=C ${CMAKE_C_COMPILER} -c ${CMAKE_CURRENT_SOURCE_DIR}/pitfalls/dos_bioscom.c -o /dev/null 2>&1 | grep -q 'bios.h'")
endif()
