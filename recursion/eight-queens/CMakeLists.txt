cmake_minimum_required(VERSION 3.16)
project(eight_queens C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)

add_library(queens_core STATIC src/queens.c)
add_executable(queens src/queens_cli.c)
add_executable(queens_stats src/queens_stats.c)
add_executable(test_queens tests/test_queens.c)
add_executable(pitfall_never_stops pitfalls/never_stops.c)
target_link_libraries(queens queens_core)
target_link_libraries(queens_stats queens_core)
target_link_libraries(test_queens queens_core)

enable_testing()
add_test(NAME test_queens COMMAND test_queens)

function(expect_output name expected prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}"
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()

expect_output(queens_8_output queens_8.txt $<TARGET_FILE:queens>)
expect_output(queens_3_output queens_3.txt $<TARGET_FILE:queens> 3)
expect_output(queens_stats_output queens_stats.txt $<TARGET_FILE:queens_stats>)
expect_output(pitfall_never_stops_output pitfall_never_stops.txt $<TARGET_FILE:pitfall_never_stops>)

add_test(NAME rejects_zero COMMAND queens 0)
add_test(NAME rejects_too_big COMMAND queens 17)
add_test(NAME rejects_letters COMMAND queens abc)
add_test(NAME rejects_sign COMMAND queens +8)
add_test(NAME rejects_two_args COMMAND queens 8 8)
set_tests_properties(rejects_zero rejects_too_big rejects_letters rejects_sign
                     rejects_two_args PROPERTIES WILL_FAIL TRUE)

# The SDL2 viewer is optional: built only where SDL2 is installed.
find_package(SDL2 CONFIG QUIET)
if(SDL2_FOUND)
  add_executable(queens_view src/queens_view.c)
  target_link_libraries(queens_view queens_core SDL2::SDL2)
  add_executable(test_screenshot tests/test_screenshot.c)
  target_link_libraries(test_screenshot SDL2::SDL2)
  set(shot ${CMAKE_CURRENT_BINARY_DIR}/solution1.bmp)
  add_test(NAME view_screenshot COMMAND queens_view 8 --screenshot 1 ${shot})
  add_test(NAME view_screenshot_check COMMAND test_screenshot ${shot})
  set_tests_properties(view_screenshot_check PROPERTIES DEPENDS view_screenshot)
  add_test(NAME view_rejects_solution_93 COMMAND queens_view 8 --screenshot 93 ${shot}.x)
  set_tests_properties(view_rejects_solution_93 PROPERTIES WILL_FAIL TRUE)
  message(STATUS "SDL2 ${SDL2_VERSION} found: building queens_view")
else()
  message(STATUS "SDL2 not found: skipping queens_view")
endif()
