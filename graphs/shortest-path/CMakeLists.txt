cmake_minimum_required(VERSION 3.16)
project(shortest_path C CXX)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen, sscanf
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(shortest_path STATIC src/graph.c src/shortest_path.c)

add_executable(route src/route.c)                     # route MAP FROM TO [dijkstra|floyd]
add_executable(trace src/trace.c)                     # step-by-step walkthroughs
add_executable(test_shortest_path tests/test_shortest_path.c)
add_executable(timing bench/timing.c)                 # built, not run by ctest
foreach(t route trace test_shortest_path timing)
  target_link_libraries(${t} PRIVATE shortest_path)
endforeach()

add_executable(route_cpp cpp/route.cpp)               # the C++17 version

# The pitfalls are wrong on purpose, but compile cleanly.
add_executable(pitfall_negative_edge pitfalls/negative_edge.c)
add_executable(pitfall_magic_infinity pitfalls/magic_infinity.c)
add_executable(pitfall_loop_order pitfalls/loop_order.c)

enable_testing()
set(MAP ${CMAKE_CURRENT_SOURCE_DIR}/data/lakeland.txt)
add_test(NAME unit_and_random_tests COMMAND test_shortest_path ${MAP})

function(expect_output name expected rc prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}" -DEXPECT_RC=${rc}
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()

# The C and C++ programs must print the same thing, byte for byte.
foreach(prog route route_cpp)
  expect_output(${prog}_dijkstra route_dijkstra.txt 0 $<TARGET_FILE:${prog}> "${MAP},Ashford,Larkspur")
  expect_output(${prog}_floyd route_floyd.txt 0 $<TARGET_FILE:${prog}> "${MAP},Larkspur,Ashford,floyd")
  expect_output(${prog}_no_route no_route.txt 1 $<TARGET_FILE:${prog}> "${MAP},Ashford,Seal Rock")
  expect_output(${prog}_unknown_town empty.txt 2 $<TARGET_FILE:${prog}> "${MAP},Ashford,Atlantis")
  expect_output(${prog}_from from_ashford.txt 0 $<TARGET_FILE:${prog}> "${MAP},--from,Ashford")
  expect_output(${prog}_table table.txt 0 $<TARGET_FILE:${prog}> "${MAP},--table")
  expect_output(${prog}_bad_map empty.txt 2 $<TARGET_FILE:${prog}>
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/bad_map.txt,A,B")
endforeach()

expect_output(trace_dijkstra trace_dijkstra.txt 0 $<TARGET_FILE:trace> "${MAP},dijkstra,Ashford")
expect_output(trace_floyd trace_floyd.txt 0 $<TARGET_FILE:trace>
              "${CMAKE_CURRENT_SOURCE_DIR}/data/four_towns.txt,floyd")
expect_output(negative_edge_output negative_edge.txt 0 $<TARGET_FILE:pitfall_negative_edge>)
expect_output(magic_infinity_output magic_infinity.txt 0 $<TARGET_FILE:pitfall_magic_infinity>)
expect_output(loop_order_output loop_order.txt 0 $<TARGET_FILE:pitfall_loop_order>)
