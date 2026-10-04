# Run PROG and compare its stdout with EXPECTED. Carriage returns are
# removed first: on Windows, text-mode output writes \r\n line endings.
# With SKIP_QSORT set, the "qsort() ..." row is removed from both sides
# before comparing: the expected files record glibc's qsort(), and other
# C libraries (MSVC, macOS, musl) use other algorithms, so their counts and
# stability differ. Every other row is still compared byte for byte.
execute_process(COMMAND ${PROG} OUTPUT_VARIABLE out RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "${PROG} exited with ${rc}")
endif()
file(READ ${EXPECTED} want)
string(REPLACE "\r" "" out "${out}")
string(REPLACE "\r" "" want "${want}")
if(SKIP_QSORT)
  string(REGEX REPLACE "\nqsort\\(\\)[^\n]*" "" out "${out}")
  string(REGEX REPLACE "\nqsort\\(\\)[^\n]*" "" want "${want}")
  message(STATUS "not glibc: the qsort() row was left out of the comparison")
endif()
if(NOT out STREQUAL want)
  message(FATAL_ERROR "output of ${PROG} differs from ${EXPECTED}:\n${out}")
endif()
