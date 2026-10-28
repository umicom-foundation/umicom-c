#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli.cmake
#
# PURPOSE:
#   Verify real CLI artifacts, error exits and refusal to overwrite existing output.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------
if(NOT DEFINED TOOL OR NOT DEFINED WORK)
    message(FATAL_ERROR "The test requires explicit TOOL and WORK paths")
endif()
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/source.uc" "int main(void) { return 6 * 7; }\n")
# This exact path belongs to this regression fixture. No directory is removed.
file(REMOVE "${WORK}/program.umcb")
execute_process(COMMAND "${TOOL}" compile "${WORK}/source.uc" "${WORK}/program.umcb"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT EXISTS "${WORK}/program.umcb")
    message(FATAL_ERROR "Compile failed: ${result}\n${output}\n${error}")
endif()
file(SHA256 "${WORK}/program.umcb" original)
execute_process(COMMAND "${TOOL}" compile "${WORK}/source.uc" "${WORK}/program.umcb"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(SHA256 "${WORK}/program.umcb" after)
if(NOT result EQUAL 2 OR NOT original STREQUAL after)
    message(FATAL_ERROR "Existing artifact was not preserved")
endif()
execute_process(COMMAND "${TOOL}" exec "${WORK}/program.umcb"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Result: 42")
    message(FATAL_ERROR "Bytecode execution failed: ${result}\n${output}\n${error}")
endif()
execute_process(COMMAND "${TOOL}" disassemble "${WORK}/program.umcb"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "multiply" OR NOT output MATCHES "return")
    message(FATAL_ERROR "Disassembly failed: ${result}\n${output}\n${error}")
endif()
file(WRITE "${WORK}/invalid.uc" "int main(void) { return 1 / 0; }\n")
execute_process(COMMAND "${TOOL}" run "${WORK}/invalid.uc"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 1 OR NOT error MATCHES "arithmetic overflow")
    message(FATAL_ERROR "Arithmetic failure was not reported: ${result}\n${output}\n${error}")
endif()
file(WRITE "${WORK}/invalid.uc" "int main(void) { return 1; } unexpected\n")
execute_process(COMMAND "${TOOL}" check "${WORK}/invalid.uc"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 1)
    message(FATAL_ERROR "Trailing source was not rejected: ${result}\n${output}\n${error}")
endif()
