cmake_minimum_required(VERSION 3.16)
project(porter_stemmer C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen, strcpy
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(porter STATIC src/porter.c)

add_executable(stem src/stem.c)                 # one word per line in, one stem out
add_executable(stem_demo src/stem_demo.c)       # the article's listing
add_executable(stem_stats src/stem_stats.c)
add_executable(test_porter tests/test_porter.c)
foreach(t stem stem_demo stem_stats test_porter)
  target_link_libraries(${t} PRIVATE porter)
endforeach()
if(NOT WIN32)                                   # clock_gettime; built, not run by ctest
  add_executable(stem_speed bench/stem_speed.c)
  target_link_libraries(stem_speed PRIVATE porter)
endif()

enable_testing()
set(V ${CMAKE_CURRENT_SOURCE_DIR}/tests/vectors)
add_test(NAME vocabulary_and_units COMMAND test_porter
         ${V}/voc.txt ${V}/paper_output.txt ${V}/standard_output.txt)

function(expect_output name expected prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}"
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
expect_output(demo_output stem_demo.txt $<TARGET_FILE:stem_demo>)
expect_output(trace_output trace_generalizations.txt $<TARGET_FILE:stem> "--trace,generalizations")
expect_output(paper_trace_output trace_possibly_paper.txt $<TARGET_FILE:stem> "--paper,--trace,possibly")
expect_output(stats_output stem_stats.txt $<TARGET_FILE:stem_stats> ${V}/voc.txt)
expect_output(stem_file_standard ../vectors/standard_output.txt $<TARGET_FILE:stem> ${V}/voc.txt)
expect_output(stem_file_paper ../vectors/paper_output.txt $<TARGET_FILE:stem> "--paper,${V}/voc.txt")

# Bad command lines and missing files are rejected (non-zero exit status).
add_test(NAME rejects_unknown_option COMMAND stem --nonsense extra)
add_test(NAME rejects_trace_without_word COMMAND stem --trace)
add_test(NAME rejects_missing_file COMMAND stem no-such-file.txt)
set_tests_properties(rejects_unknown_option rejects_trace_without_word rejects_missing_file
                     PROPERTIES WILL_FAIL TRUE)
