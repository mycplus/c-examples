cmake_minimum_required(VERSION 3.16)
project(brick_game_c C)

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(include)
add_library(breakout STATIC src/breakout.c src/screen.c src/autopilot.c)

add_executable(brick       src/main.c src/term.c)
add_executable(autoplay    src/autoplay.c)
add_executable(tick_timing src/tick_timing.c src/term.c)
target_link_libraries(brick breakout)
target_link_libraries(autoplay breakout)

add_executable(test_breakout tests/test_breakout.c)
add_executable(test_screen   tests/test_screen.c)
target_link_libraries(test_breakout breakout)
target_link_libraries(test_screen breakout)

enable_testing()
add_test(NAME rules  COMMAND test_breakout)
add_test(NAME screen COMMAND test_screen)
# The article's autoplay output, checked byte for byte
add_test(NAME output_autoplay
         COMMAND ${CMAKE_COMMAND} -DPROG=$<TARGET_FILE:autoplay>
                 -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/autoplay.txt
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
