cmake_minimum_required(VERSION 3.18)
project(libgd_examples C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen, sscanf
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# LibGD: pkg-config on Linux and macOS; on Windows, vcpkg's toolchain file
# puts gd.h and libgd.lib where find_path() and find_library() look.
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(GD IMPORTED_TARGET gdlib)
endif()
if(TARGET PkgConfig::GD)
  set(GD_TARGET PkgConfig::GD)
else()
  find_path(GD_INCLUDE_DIR gd.h REQUIRED)
  find_library(GD_LIBRARY NAMES gd libgd REQUIRED)
  add_library(gd_lib UNKNOWN IMPORTED)
  set_target_properties(gd_lib PROPERTIES IMPORTED_LOCATION ${GD_LIBRARY}
                        INTERFACE_INCLUDE_DIRECTORIES ${GD_INCLUDE_DIR})
  set(GD_TARGET gd_lib)
endif()

add_executable(first_image src/first_image.c)
target_link_libraries(first_image PRIVATE ${GD_TARGET})

add_library(imgtools STATIC src/imgtools.c)
target_include_directories(imgtools PUBLIC src)
target_link_libraries(imgtools PUBLIC ${GD_TARGET})
foreach(p chart thumbnail identicon imgcheck)
  add_executable(${p} src/${p}.c)
  target_link_libraries(${p} PRIVATE imgtools)
endforeach()
add_executable(test_imgtools tests/test_imgtools.c)
target_link_libraries(test_imgtools PRIVATE imgtools)
foreach(p text_mode palette_limits png_buffer_leak unchecked_load)
  add_executable(pitfall_${p} pitfalls/${p}.c)
  target_link_libraries(pitfall_${p} PRIVATE ${GD_TARGET})
endforeach()

# ---- Tests ----
enable_testing()
set(T ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(DATA ${CMAKE_CURRENT_SOURCE_DIR}/data/downloads.txt)
function(expect_output name expected rc prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}" -DEXPECT_RC=${rc} -DEXPECTED=${T}/expected/${expected}
    -P ${T}/compare_output.cmake)
endfunction()

add_test(NAME unit_tests COMMAND test_imgtools ${DATA})

# Each program writes an image; imgcheck then reads it back and checks its
# format, size and a few pixels.
expect_output(first_image first_image.txt 0 $<TARGET_FILE:first_image>)
expect_output(first_image_files first_image_files.txt 0 $<TARGET_FILE:imgcheck>
              "first.png,0,0,90,120,300,180")
expect_output(first_image_jpeg first_image_jpeg.txt 0 $<TARGET_FILE:imgcheck> "first.jpg")
set_tests_properties(first_image PROPERTIES FIXTURES_SETUP first)
set_tests_properties(first_image_files first_image_jpeg PROPERTIES FIXTURES_REQUIRED first)

expect_output(chart chart.txt 0 $<TARGET_FILE:chart> "${DATA},chart.png")
expect_output(chart_file chart_file.txt 0 $<TARGET_FILE:imgcheck> "chart.png,0,0,358,258")
set_tests_properties(chart PROPERTIES FIXTURES_SETUP chart)
set_tests_properties(chart_file PROPERTIES FIXTURES_REQUIRED chart)
expect_output(chart_no_data empty.txt 2 $<TARGET_FILE:chart> "${T}/expected/empty.txt,x.png")

expect_output(identicon identicon-alice.txt 0 $<TARGET_FILE:identicon> "alice,alice.png")
expect_output(identicon_file identicon-alice-file.txt 0 $<TARGET_FILE:imgcheck> "alice.png,0,0,50,50,75,75")
set_tests_properties(identicon PROPERTIES FIXTURES_SETUP identicon)
set_tests_properties(identicon_file PROPERTIES FIXTURES_REQUIRED identicon)

expect_output(thumbnail thumbnail.txt 0 $<TARGET_FILE:thumbnail> "first.jpg,thumb.jpg,100")
expect_output(thumbnail_png thumbnail-png.txt 0 $<TARGET_FILE:thumbnail> "first.png,thumb.png,64")
expect_output(thumbnail_file thumbnail-file.txt 0 $<TARGET_FILE:imgcheck> "thumb.jpg")
expect_output(thumbnail_rejects_huge empty.txt 1 $<TARGET_FILE:thumbnail> "${T}/files/huge.png,x.jpg,100")
expect_output(thumbnail_rejects_text empty.txt 1 $<TARGET_FILE:thumbnail> "${DATA},x.jpg,100")
expect_output(thumbnail_bad_args empty.txt 2 $<TARGET_FILE:thumbnail> "first.jpg,x.gif,100")
set_tests_properties(thumbnail thumbnail_png PROPERTIES FIXTURES_REQUIRED first FIXTURES_SETUP thumb)
set_tests_properties(thumbnail_file PROPERTIES FIXTURES_REQUIRED thumb)

if(WIN32)
  expect_output(pitfall_text_mode text_mode_windows.txt 0 $<TARGET_FILE:pitfall_text_mode>)
else()
  expect_output(pitfall_text_mode text_mode.txt 0 $<TARGET_FILE:pitfall_text_mode>)
endif()
expect_output(pitfall_palette_limits palette_limits.txt 0 $<TARGET_FILE:pitfall_palette_limits>)
expect_output(pitfall_png_buffer png_buffer.txt 0 $<TARGET_FILE:pitfall_png_buffer_leak>)
expect_output(pitfall_unchecked_load unchecked_load.txt 1 $<TARGET_FILE:pitfall_unchecked_load>
              "${T}/files/huge.png")

# The title in a TrueType font, when a font is available on this system.
find_file(CHART_FONT NAMES DejaVuSans.ttf arial.ttf
          PATHS /usr/share/fonts/truetype/dejavu /usr/share/fonts/TTF "C:/Windows/Fonts" NO_DEFAULT_PATH)
if(CHART_FONT)
  expect_output(chart_truetype chart-truetype.txt 0 $<TARGET_FILE:chart> "${DATA},chart-ft.png,${CHART_FONT}")
endif()
