cmake_minimum_required(VERSION 3.16)
project(directory_listing C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # strcpy, gmtime, localtime
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
if(WIN32)
  set(FS_PLATFORM src/fs_win32.c)
else()
  set(FS_PLATFORM src/fs_posix.c)
endif()
add_library(fsdir STATIC src/fs_common.c src/walk.c ${FS_PLATFORM})

foreach(p dirlist tree bigfiles make_fixture)
  add_executable(${p} src/${p}.c)
  target_link_libraries(${p} PRIVATE fsdir)
endforeach()
add_executable(test_fs tests/test_fs.c)
target_link_libraries(test_fs PRIVATE fsdir)
add_executable(pitfall_path_buffer pitfalls/path_buffer.c)
target_link_libraries(pitfall_path_buffer PRIVATE fsdir)
# The overflow is the lesson. GCC spots it when optimizing (-Warray-bounds),
# so its warning is shown but does not stop the build.
target_compile_options(pitfall_path_buffer PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/WX-,-Wno-error>)
if(WIN32)
  add_executable(minimal_win32 src/minimal_win32.c)
else()
  add_executable(minimal_posix src/minimal_posix.c)
endif()
if(NOT WIN32)
  add_executable(pitfall_stat_wrong_path pitfalls/stat_wrong_path.c)
  add_executable(pitfall_symlink_loop pitfalls/symlink_loop.c)
endif()

enable_testing()
# Every test runs in the build directory, where the fixture is created.
add_test(NAME make_fixture COMMAND make_fixture fixture WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(make_fixture PROPERTIES FIXTURES_SETUP tree)

add_test(NAME unit_tests COMMAND test_fs fixture scratch WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(unit_tests PROPERTIES FIXTURES_REQUIRED tree)

function(expect_output name expected rc prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}" -DEXPECT_RC=${rc}
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED tree)
endfunction()
expect_output(dirlist_default dirlist.txt 0 $<TARGET_FILE:dirlist> "-u,fixture")
expect_output(dirlist_all dirlist_all.txt 0 $<TARGET_FILE:dirlist> "-a,-u,fixture")
expect_output(dirlist_dirs dirlist_dirs.txt 0 $<TARGET_FILE:dirlist> "-d,-u,fixture")
expect_output(dirlist_files dirlist_files.txt 0 $<TARGET_FILE:dirlist> "-f,-u,fixture/docs")
expect_output(dirlist_missing empty.txt 2 $<TARGET_FILE:dirlist> "no-such-directory")
expect_output(tree_default tree.txt 0 $<TARGET_FILE:tree> "fixture")
expect_output(tree_sizes tree_sizes.txt 0 $<TARGET_FILE:tree> "-a,-s,fixture")
expect_output(tree_one_level tree_level1.txt 0 $<TARGET_FILE:tree> "-L,1,fixture")
expect_output(tree_one_level_sizes tree_level1_sizes.txt 0 $<TARGET_FILE:tree> "-s,-L,1,fixture")
expect_output(bigfiles_top3 bigfiles.txt 0 $<TARGET_FILE:bigfiles> "fixture,3")
expect_output(path_buffer_checked path_buffer.txt 0 $<TARGET_FILE:pitfall_path_buffer> "checked")
# The minimal listers print in whatever order the system returns, so they only have to run.
if(WIN32)
  add_test(NAME minimal_runs COMMAND minimal_win32 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
  add_test(NAME minimal_runs COMMAND minimal_posix fixture WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
set_tests_properties(minimal_runs PROPERTIES FIXTURES_REQUIRED tree)
if(NOT WIN32)
  expect_output(stat_wrong_path stat_wrong_path.txt 0 $<TARGET_FILE:pitfall_stat_wrong_path> "fixture")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")    # the 40-link limit is Linux's
    expect_output(symlink_loop symlink_loop.txt 0 $<TARGET_FILE:pitfall_symlink_loop> "scratch")
    set_tests_properties(symlink_loop PROPERTIES DEPENDS unit_tests)   # unit_tests creates scratch
  endif()
endif()
