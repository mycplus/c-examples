cmake_minimum_required(VERSION 3.16)
project(sequence_alignment C)

set(CMAKE_C_STANDARD 11 CACHE STRING "C standard")
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)   # fopen
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
add_library(align STATIC src/align.c src/blosum.c)

add_executable(align_demo src/align_demo.c)      # the article's examples
add_executable(seqalign src/seqalign.c)          # seqalign [options] FILE1 FILE2
add_executable(test_align tests/test_align.c)
foreach(t align_demo seqalign test_align)
  target_link_libraries(${t} PRIVATE align)
endforeach()
add_executable(pitfall_local_without_zero pitfalls/local_without_zero.c)
if(NOT WIN32)
  # Reads past the end of a table on purpose; the Linux workflow checks that
  # AddressSanitizer catches it.
  add_executable(pitfall_letter_index pitfalls/letter_index.c)
  add_executable(align_speed bench/align_speed.c)  # built, not run by ctest
  target_link_libraries(align_speed PRIVATE align)
endif()

enable_testing()
add_test(NAME unit_and_brute_force_tests COMMAND test_align)

function(expect_output name expected rc prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}" -DEXPECT_RC=${rc}
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
set(D ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)
expect_output(demo_output align_demo.txt 0 $<TARGET_FILE:align_demo>)
expect_output(seqalign_local_dna seqalign_dna.txt 0 $<TARGET_FILE:seqalign>
              "-local,-matrix,DNA,-open,0,-extend,2,${D}/dna1.fa,${D}/dna2.fa")
expect_output(seqalign_global_protein seqalign_protein.txt 0 $<TARGET_FILE:seqalign>
              "-matrix,BLOSUM50,-open,0,-extend,8,${D}/heagawghee.fa,${D}/pawheae.fa")
expect_output(local_without_zero_output local_without_zero.txt 0
              $<TARGET_FILE:pitfall_local_without_zero>)
add_test(NAME seqalign_rejects_bad_letter COMMAND seqalign -matrix DNA ${D}/rna.fa ${D}/dna1.fa)
add_test(NAME seqalign_rejects_missing_file COMMAND seqalign no-such-file.fa ${D}/dna1.fa)
set_tests_properties(seqalign_rejects_bad_letter seqalign_rejects_missing_file
                     PROPERTIES WILL_FAIL TRUE)
