# Runs PROG (with optional ARGS) with INPUT as stdin and compares its stdout with EXPECTED.
# CR characters are ignored so the same expected files pass on Windows.
separate_arguments(arg_list NATIVE_COMMAND "${ARGS}")
execute_process(COMMAND ${PROG} ${arg_list}
                INPUT_FILE ${INPUT}
                OUTPUT_VARIABLE actual
                RESULT_VARIABLE rc
                ENCODING UTF-8)
string(REPLACE "\r" "" actual "${actual}")
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "${PROG} exited with ${rc}. Output was:\n${actual}")
endif()
file(READ ${EXPECTED} expected)
string(REPLACE "\r" "" expected "${expected}")
if(NOT actual STREQUAL expected)
  message(FATAL_ERROR "Output differs from ${EXPECTED}. Actual output:\n${actual}")
endif()
