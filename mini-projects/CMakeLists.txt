cmake_minimum_required(VERSION 3.16)
project(c_mini_projects C)

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

enable_testing()

# mini_project_session(<target> <name> [ARGS "<args>"] [REMOVE <file>])
# Runs <target> with tests/<name>.txt as stdin and compares stdout with tests/<name>.expected.txt.
# Every session runs in the project's build folder, so file names in ARGS and REMOVE are
# relative to it. REMOVE deletes that file first, so a test that writes a file starts clean.
function(mini_project_session target name)
  cmake_parse_arguments(MPS "" "ARGS;REMOVE" "" ${ARGN})
  set(remove "")
  if(MPS_REMOVE)
    set(remove ${CMAKE_CURRENT_BINARY_DIR}/${MPS_REMOVE})
  endif()
  add_test(NAME ${target}_${name}
           COMMAND ${CMAKE_COMMAND}
                   -DPROG=$<TARGET_FILE:${target}>
                   -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}.txt
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}.expected.txt
                   "-DARGS=${MPS_ARGS}"
                   "-DREMOVE=${remove}"
                   -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}
                   -P ${PROJECT_SOURCE_DIR}/cmake/run_session.cmake
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

add_subdirectory(unit-converter)
add_subdirectory(profit-loss-calculator)
add_subdirectory(hangman)
add_subdirectory(swat-the-fly)
add_subdirectory(grade-report)
add_subdirectory(balloon-shooter)
add_subdirectory(sliding-puzzle)
add_subdirectory(fly-away-adventure)
add_subdirectory(mini-piano)
add_subdirectory(personal-diary)
add_subdirectory(address-book)
add_subdirectory(birthday-reminder)
add_subdirectory(todo-organizer)
add_subdirectory(library-management)
