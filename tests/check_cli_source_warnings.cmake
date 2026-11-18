#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_source_warnings.cmake
#
# PURPOSE:
#   Check hosted warning output, arity, exit status and nonexecution.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------

if(NOT DEFINED TOOL OR NOT EXISTS "${TOOL}" OR NOT DEFINED WORK)
    message(FATAL_ERROR "Provide TOOL and a private test WORK directory")
endif()
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/unused.uc" "int main(void){\nint unused=1;\nreturn 42;}\n")
file(WRITE "${WORK}/used.uc" "int main(void){int used=1;return used;}")
file(WRITE "${WORK}/fault.uc" "int main(void){int unused=1/0;return 42;}")
file(WRITE "${WORK}/bad.uc" "int main(void){int unused=1;return absent;}")
execute_process(COMMAND "${TOOL}" check "${WORK}/unused.uc" --warnings RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 0 OR NOT output MATCHES "4 instructions; 1 warnings" OR NOT error MATCHES ":2:5: warning: local .unused. is never read")
    message(FATAL_ERROR "warning report mismatch: ${status}: ${output}: ${error}")
endif()
execute_process(COMMAND "${TOOL}" check "${WORK}/unused.uc" RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 0 OR NOT output MATCHES "accepted: 4 instructions" OR output MATCHES "warnings" OR NOT error STREQUAL "")
    message(FATAL_ERROR "legacy check changed: ${status}: ${output}: ${error}")
endif()
execute_process(COMMAND "${TOOL}" check "${WORK}/used.uc" --warnings RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 0 OR NOT output MATCHES "0 warnings" OR NOT error STREQUAL "")
    message(FATAL_ERROR "used local warning mismatch: ${status}: ${output}: ${error}")
endif()
execute_process(COMMAND "${TOOL}" check "${WORK}/fault.uc" --warnings RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 0 OR NOT output MATCHES "1 warnings")
    message(FATAL_ERROR "source check executed arithmetic: ${status}: ${output}: ${error}")
endif()
execute_process(COMMAND "${TOOL}" check "${WORK}/bad.uc" --warnings RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 2 OR NOT output STREQUAL "" OR error MATCHES "warning:" OR NOT error MATCHES "declared and initialized")
    message(FATAL_ERROR "partial warnings published after syntax error: ${status}: ${output}: ${error}")
endif()
foreach(command IN ITEMS run inspect compile)
    execute_process(COMMAND "${TOOL}" "${command}" "${WORK}/used.uc" --warnings --extra RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT status EQUAL 2 OR NOT output STREQUAL "")
        message(FATAL_ERROR "invalid warning option accepted by ${command}")
    endif()
endforeach()
execute_process(COMMAND "${TOOL}" check "${WORK}/used.uc" --warnings --extra RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 2 OR NOT output STREQUAL "")
    message(FATAL_ERROR "unexpected warning argument accepted")
endif()
execute_process(COMMAND "${TOOL}" check "${WORK}/used.uc" --warning RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 2 OR NOT output STREQUAL "")
    message(FATAL_ERROR "misspelled warning option accepted")
endif()
