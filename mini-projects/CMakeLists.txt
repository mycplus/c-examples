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

# mini_project_session(<target> <name> [ARGS "<args>"])
# Runs <target> with tests/<name>.txt as stdin and compares stdout with tests/<name>.expected.txt.
function(mini_project_session target name)
  cmake_parse_arguments(MPS "" "ARGS" "" ${ARGN})
  add_test(NAME ${target}_${name}
           COMMAND ${CMAKE_COMMAND}
                   -DPROG=$<TARGET_FILE:${target}>
                   -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}.txt
                   -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}.expected.txt
                   "-DARGS=${MPS_ARGS}"
                   -P ${PROJECT_SOURCE_DIR}/cmake/run_session.cmake)
endfunction()

add_subdirectory(unit-converter)
add_subdirectory(profit-loss-calculator)
add_subdirectory(hangman)
