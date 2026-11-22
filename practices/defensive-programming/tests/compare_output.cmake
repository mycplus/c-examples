# Runs PROG with ARGS in WORKDIR and compares its standard output with EXPECTED.
# Exit codes 0 and 1 are both normal for these programs.
execute_process(COMMAND ${PROG} ${ARGS} WORKING_DIRECTORY ${WORKDIR}
  OUTPUT_VARIABLE out RESULT_VARIABLE rc)
if(NOT rc MATCHES "^[01]$")
  message(FATAL_ERROR "${PROG} failed: ${rc}")
endif()
string(REPLACE "\r\n" "\n" out "${out}")
file(READ ${EXPECTED} expected)
string(REPLACE "\r\n" "\n" expected "${expected}")
if(NOT out STREQUAL expected)
  message(FATAL_ERROR "Output differs from ${EXPECTED}\n--- actual ---\n${out}")
endif()
