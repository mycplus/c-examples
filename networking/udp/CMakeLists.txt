cmake_minimum_required(VERSION 3.16)
project(udp_c C)

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_library(udp STATIC src/udp.c)
target_include_directories(udp PUBLIC include)
if(WIN32)
  target_link_libraries(udp PUBLIC ws2_32)
endif()

foreach(prog udp-sender udp-receiver boundaries connected loss_test)
  add_executable(${prog} src/${prog}.c)
  target_link_libraries(${prog} PRIVATE udp)
endforeach()

enable_testing()
foreach(prog boundaries connected)
  add_test(NAME output_${prog}
           COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:${prog}>
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${prog}.txt
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endforeach()
# Loss counts differ by system, so only the invariants are checked: something
# arrives, nothing is corrupted or duplicated, nothing arrives out of order.
add_test(NAME loss_small_buffer COMMAND loss_test 5000 4096)
add_test(NAME loss_default_buffer COMMAND loss_test 5000 0)
add_test(NAME usage_errors COMMAND udp-sender)
set_tests_properties(usage_errors PROPERTIES WILL_FAIL TRUE)

if(NOT WIN32)
  add_test(NAME sender_and_receiver
           COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/sender_receiver_test.sh $<TARGET_FILE_DIR:udp-sender>)
endif()
