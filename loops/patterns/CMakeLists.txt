cmake_minimum_required(VERSION 3.16)
project(patterns C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # scanf, sscanf, fopen, strcpy
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(patternlib STATIC src/patterns.c)

add_executable(pyramid src/pyramid.c)
add_executable(patterns src/patterns_cli.c)
add_executable(shapes src/shapes.c)
add_executable(barchart src/barchart.c)
add_executable(test_patterns tests/test_patterns.c)
add_executable(pitfall_pascal_factorial pitfalls/pascal_factorial.c)
foreach(t patterns test_patterns pitfall_pascal_factorial)
  target_link_libraries(${t} PRIVATE patternlib)
endforeach()
foreach(p off_by_one double_middle trailing_spaces unchecked_scanf)
  add_executable(pitfall_${p} pitfalls/${p}.c)
endforeach()

enable_testing()
add_test(NAME unit_tests COMMAND test_patterns)

set(T ${CMAKE_CURRENT_SOURCE_DIR}/tests)
function(expect_output name expected rc prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}" -DEXPECT_RC=${rc} -DEXPECTED=${T}/expected/${expected}
    -P ${T}/compare_output.cmake)
endfunction()
function(expect_output_input name expected input prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} -DINPUT=${T}/${input} -DEXPECTED=${T}/expected/${expected}
    -P ${T}/compare_output.cmake)
endfunction()

# Every pattern at size 4, compared byte for byte.
foreach(p square hollow-square right-triangle inverted-right-triangle mirrored-right-triangle
          pyramid inverted-pyramid hollow-pyramid diamond hollow-diamond hourglass
          number-triangle repeated-number-triangle floyd pascal alphabet-triangle)
  expect_output(pattern_${p} ${p}.txt 0 $<TARGET_FILE:patterns> "${p},4")
endforeach()
expect_output(pattern_pascal_7 pascal-7.txt 0 $<TARGET_FILE:patterns> "pascal,7")
expect_output(pattern_floyd_5 floyd-5.txt 0 $<TARGET_FILE:patterns> "floyd,5")
expect_output(pattern_list list.txt 0 $<TARGET_FILE:patterns> "list")
expect_output(pattern_unknown empty.txt 2 $<TARGET_FILE:patterns> "triangle,4")
expect_output(pattern_too_big empty.txt 2 $<TARGET_FILE:patterns> "alphabet-triangle,27")
expect_output(pattern_not_a_number empty.txt 2 $<TARGET_FILE:patterns> "pyramid,4x")

foreach(s diamond hollow-diamond x plus circle triangle)
  expect_output(shape_${s} shape-${s}.txt 0 $<TARGET_FILE:shapes> "${s},3")
endforeach()

expect_output_input(pyramid_5 pyramid-5.txt input-5.txt $<TARGET_FILE:pyramid>)
add_test(NAME pyramid_rejects_text COMMAND ${CMAKE_COMMAND}
  -DPROG=$<TARGET_FILE:pyramid> -DINPUT=${T}/input-four.txt -DEXPECT_RC=1
  -DEXPECTED=${T}/expected/pyramid-rejected.txt -P ${T}/compare_output.cmake)
expect_output(barchart_downloads barchart.txt 0 $<TARGET_FILE:barchart>
              "${CMAKE_CURRENT_SOURCE_DIR}/data/downloads.txt")

expect_output(off_by_one off_by_one.txt 0 $<TARGET_FILE:pitfall_off_by_one>)
expect_output(double_middle double_middle.txt 0 $<TARGET_FILE:pitfall_double_middle>)
expect_output(trailing_spaces trailing_spaces.txt 0 $<TARGET_FILE:pitfall_trailing_spaces>)
expect_output_input(unchecked_scanf unchecked_scanf.txt input-four.txt $<TARGET_FILE:pitfall_unchecked_scanf>)
expect_output(pascal_factorial pascal_factorial.txt 0 $<TARGET_FILE:pitfall_pascal_factorial>)
