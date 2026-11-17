cmake_minimum_required(VERSION 3.21)
project(enums C CXX)

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
add_library(order STATIC src/order.c)

foreach(p basics underlying flags log_levels)
  add_executable(${p} src/${p}.c)
endforeach()
add_executable(order_demo src/order_demo.c)
add_executable(test_enums tests/test_enums.c)
add_executable(pitfall_out_of_range pitfalls/out_of_range.c)
foreach(t order_demo test_enums pitfall_out_of_range)
  target_link_libraries(${t} PRIVATE order)
endforeach()
add_executable(pitfall_reorder pitfalls/reorder.c)
add_executable(enum_class cpp/enum_class.cpp)

# The missing switch case is shown running, so its warning may not stop the build.
add_executable(pitfall_missing_case pitfalls/missing_case.c)
target_compile_options(pitfall_missing_case PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/WX-,-Wno-error>)

# C23 fixed underlying types: GCC 13+ and Clang 18+; not MSVC's C compiler yet.
include(CheckCSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-std=c2x")
check_c_source_compiles("enum e : unsigned char { A }; int main(void) { return A; }" HAVE_C23_ENUM)
unset(CMAKE_REQUIRED_FLAGS)
if(HAVE_C23_ENUM AND NOT MSVC)
  add_executable(c23_enum src/c23_enum.c)
  set_target_properties(c23_enum PROPERTIES C_STANDARD 23)
endif()

# Programs that must not compile. They are left out of the normal build and
# built one at a time by the tests below.
add_executable(fail_name_clash EXCLUDE_FROM_ALL compile_fail/name_clash.c)
add_executable(fail_const_case EXCLUDE_FROM_ALL compile_fail/const_case.c)
add_executable(fail_enum_class_to_int EXCLUDE_FROM_ALL compile_fail/enum_class_to_int.cpp)
add_executable(fail_int_to_enum EXCLUDE_FROM_ALL compile_fail/int_to_enum.cpp)
add_executable(fail_missing_case EXCLUDE_FROM_ALL pitfalls/missing_case.c)
if(MSVC)
  target_compile_options(fail_missing_case PRIVATE /we4062)   # C4062 is off by default, even at /W4
else()
  target_compile_options(fail_missing_case PRIVATE -Werror=switch)
endif()

enable_testing()
add_test(NAME unit_tests COMMAND test_enums)

function(expect_output name expected rc prog)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
    -DPROG=${prog} "-DARGS=${ARGN}" -DEXPECT_RC=${rc}
    -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/expected/${expected}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.cmake)
endfunction()
expect_output(basics_output basics.txt 0 $<TARGET_FILE:basics>)
expect_output(flags_output flags.txt 0 $<TARGET_FILE:flags>)
expect_output(log_levels_output log_levels.txt 0 $<TARGET_FILE:log_levels>)
expect_output(log_levels_warning log_levels_warning.txt 0 $<TARGET_FILE:log_levels> warning)
expect_output(order_demo_output order_demo.txt 0 $<TARGET_FILE:order_demo>
              ${CMAKE_CURRENT_SOURCE_DIR}/data/events.txt)
expect_output(out_of_range_checked out_of_range_checked.txt 0 $<TARGET_FILE:pitfall_out_of_range> checked)
expect_output(reorder_output reorder.txt 0 $<TARGET_FILE:pitfall_reorder>)
expect_output(missing_case_output missing_case.txt 0 $<TARGET_FILE:pitfall_missing_case>)
expect_output(enum_class_output enum_class.txt 0 $<TARGET_FILE:enum_class>)
if(TARGET c23_enum)
  expect_output(c23_enum_output c23_enum.txt 0 $<TARGET_FILE:c23_enum>)
endif()
# underlying's output is implementation-defined, so it only has to run.
add_test(NAME underlying_runs COMMAND underlying)

function(expect_build_failure target expect)
  add_test(NAME ${target} COMMAND ${CMAKE_COMMAND}
    -DBUILD_DIR=${CMAKE_BINARY_DIR} -DTARGET=${target} -DCONFIG=$<CONFIG> "-DEXPECT=${expect}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_build_failure.cmake)
endfunction()
expect_build_failure(fail_name_clash "NONE")
expect_build_failure(fail_const_case "case|constant")
expect_build_failure(fail_enum_class_to_int "Signal")
expect_build_failure(fail_int_to_enum "Color")
expect_build_failure(fail_missing_case "REFUNDED")
