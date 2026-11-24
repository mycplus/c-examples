cmake_minimum_required(VERSION 3.16)
project(bank_management C)

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

# Everything except main.c goes into a library, so the unit tests can link
# the very same code without the menu. That is the practical reason for
# splitting the program into modules.
add_library(bankcore STATIC
  src/bank.c src/date.c src/input.c src/ledger.c src/money.c src/report.c)
target_include_directories(bankcore PUBLIC src)

add_executable(bank_management src/main.c)
target_link_libraries(bank_management PRIVATE bankcore)

add_executable(unit_tests tests/unit_tests.c)
target_link_libraries(unit_tests PRIVATE bankcore)

enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)

# bank_case(<name> [ARGS "<args>"] [INPUT <file>] [OUT <file> | NO_OUT] [ERR <file>] [ERR_PREFIX]
#           [EXIT <code>] [COPY <file>] [REMOVE <file>] [DEPENDS <case>] [DIR <folder>])
# Runs bank_management in the build folder (or in the subfolder DIR) and compares
# stdout with OUT, stderr with ERR and the exit code with EXIT (default 0).
# INPUT and OUT default to tests/<name>.txt and tests/<name>.expected.txt when
# those files exist. Paths in INPUT, OUT, ERR and COPY are relative to tests/.
function(bank_case name)
  cmake_parse_arguments(C "ERR_PREFIX;NO_OUT" "ARGS;INPUT;OUT;ERR;EXIT;COPY;REMOVE;DEPENDS;DIR" "" ${ARGN})
  set(tests ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  if(NOT C_INPUT AND EXISTS ${tests}/${name}.txt)
    set(C_INPUT ${name}.txt)
  endif()
  if(NOT C_OUT AND NOT C_NO_OUT AND EXISTS ${tests}/${name}.expected.txt)
    set(C_OUT ${name}.expected.txt)
  endif()
  set(defs -DPROG=$<TARGET_FILE:bank_management> "-DARGS=${C_ARGS}"
           -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/${C_DIR})
  if(C_INPUT)
    list(APPEND defs -DINPUT=${tests}/${C_INPUT})
  endif()
  if(C_OUT)
    list(APPEND defs -DEXPECTED_OUT=${tests}/${C_OUT})
  endif()
  if(C_ERR)
    list(APPEND defs -DEXPECTED_ERR=${tests}/${C_ERR})
  endif()
  if(C_ERR_PREFIX)
    list(APPEND defs -DERR_PREFIX=1)
  endif()
  if(C_EXIT)
    list(APPEND defs -DEXPECT_EXIT=${C_EXIT})
  endif()
  if(C_COPY)
    list(APPEND defs -DCOPY=${tests}/${C_COPY})
  endif()
  if(C_REMOVE)
    list(APPEND defs -DREMOVE=${C_REMOVE})
  endif()
  add_test(NAME ${name}
           COMMAND ${CMAKE_COMMAND} ${defs} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_case.cmake)
  if(C_DEPENDS)
    set_tests_properties(${name} PROPERTIES DEPENDS ${C_DEPENDS})
  endif()
endfunction()

# Sessions: typed input in, exact output compared.
bank_case(session ARGS "--file bank-session.txt --today 2026-12-01" REMOVE bank-session.txt)
bank_case(session_verify ARGS "--verify --file bank-session.txt" DEPENDS session)

# One ledger used by three runs, in order: the second loads what the first
# wrote, and the third is refused because its date is earlier than the file's.
bank_case(persist1 ARGS "--file bank-persist.txt --today 2026-12-30" REMOVE bank-persist.txt)
bank_case(persist2 ARGS "--today 2027-01-31 --file bank-persist.txt" DEPENDS persist1)
bank_case(persist3 ARGS "--file bank-persist.txt --today 2027-01-15" NO_OUT ERR persist3.expected.txt
          EXIT 2 DEPENDS persist2)

# End of input in the middle of opening an account: no ledger file is created.
bank_case(eof ARGS "--file bank-eof.txt --today 2026-12-01" REMOVE bank-eof.txt)
bank_case(eof_nofile ARGS "--verify --file bank-eof.txt" NO_OUT ERR eof_nofile.expected.txt ERR_PREFIX
          EXIT 2 DEPENDS eof)

# A holder name with a control character (ESC) is refused with its own message.
bank_case(text_control ARGS "--file bank-text.txt --today 2026-12-01" REMOVE bank-text.txt)

# Command line mistakes.
bank_case(usage ARGS "--today 2027-02-29" NO_OUT ERR usage.expected.txt EXIT 1)

# Damaged ledgers: each file is the session ledger with one change, checked
# with --verify in its own folder. The loader must stop at the damaged line.
foreach(broken bad-header torn-line gap balance-mismatch overdraft daily-limit closed
               unknown-account wrong-number double-interest wrong-interest close-not-empty
               date-backwards bad-type fields pipe-text control-text)
  bank_case(data_${broken} ARGS "--verify --file ledger.txt" COPY data/${broken}.txt
            ERR data/${broken}.expected.txt EXIT 2 DIR data-${broken})
endforeach()
