# Runs one test case of the bank program and compares what it printed.
#
#   PROG          the program to run
#   ARGS          its arguments, as one string separated by spaces
#   WORKDIR       the folder to run it in
#   INPUT         optional: a file fed to stdin (otherwise stdin is empty)
#   EXPECTED_OUT  optional: the exact expected stdout
#   EXPECTED_ERR  optional: the exact expected stderr
#   ERR_PREFIX    optional: 1 = stderr only has to START with EXPECTED_ERR
#                 (for messages that end in the system's own error text)
#   EXPECT_EXIT   the expected exit code (default 0)
#   COPY          optional: a file copied into WORKDIR as ledger.txt first
#   REMOVE        optional: a file in WORKDIR deleted first, so the case starts clean
#
# CR characters are removed before comparing, so the same expected files
# pass on Windows, where the console writes CR LF.

if(NOT DEFINED EXPECT_EXIT OR EXPECT_EXIT STREQUAL "")
  set(EXPECT_EXIT 0)
endif()
file(MAKE_DIRECTORY "${WORKDIR}")
if(REMOVE)
  file(REMOVE "${WORKDIR}/${REMOVE}")
endif()
if(COPY)
  configure_file("${COPY}" "${WORKDIR}/ledger.txt" COPYONLY)
endif()
if(NOT INPUT)
  set(INPUT "${WORKDIR}/empty-input.txt")
  file(WRITE "${INPUT}" "")
endif()

separate_arguments(arg_list UNIX_COMMAND "${ARGS}")
execute_process(COMMAND "${PROG}" ${arg_list}
                WORKING_DIRECTORY "${WORKDIR}"
                INPUT_FILE "${INPUT}"
                OUTPUT_VARIABLE out
                ERROR_VARIABLE err
                RESULT_VARIABLE rc
                ENCODING UTF-8)
string(REPLACE "\r" "" out "${out}")
string(REPLACE "\r" "" err "${err}")

set(problems "")
if(NOT rc STREQUAL EXPECT_EXIT)
  string(APPEND problems "exit code ${rc}, expected ${EXPECT_EXIT}\n")
endif()
if(EXPECTED_OUT)
  file(READ "${EXPECTED_OUT}" want)
  string(REPLACE "\r" "" want "${want}")
  if(NOT out STREQUAL want)
    string(APPEND problems "stdout differs from ${EXPECTED_OUT}\n")
  endif()
endif()
if(EXPECTED_ERR)
  file(READ "${EXPECTED_ERR}" want)
  string(REPLACE "\r" "" want "${want}")
  if(ERR_PREFIX)
    string(LENGTH "${want}" n)
    string(SUBSTRING "${err}" 0 ${n} start)
    if(NOT start STREQUAL want)
      string(APPEND problems "stderr does not start with the text of ${EXPECTED_ERR}\n")
    endif()
  elseif(NOT err STREQUAL want)
    string(APPEND problems "stderr differs from ${EXPECTED_ERR}\n")
  endif()
endif()

if(problems)
  message(FATAL_ERROR "${problems}--- stdout ---\n${out}--- stderr ---\n${err}--- exit code: ${rc}")
endif()
