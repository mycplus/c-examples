# Run PROG and compare its stdout with EXPECTED. Carriage returns are
# removed first: on Windows, text-mode output writes \r\n line endings.
execute_process(COMMAND ${PROG} OUTPUT_VARIABLE out RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "${PROG} exited with ${rc}")
endif()
file(READ ${EXPECTED} want)
string(REPLACE "\r" "" out "${out}")
string(REPLACE "\r" "" want "${want}")
if(NOT out STREQUAL want)
  message(FATAL_ERROR "output of ${PROG} differs from ${EXPECTED}:\n${out}")
endif()
