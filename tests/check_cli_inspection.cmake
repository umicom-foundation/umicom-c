#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_inspection.cmake
#
# PURPOSE:
#   Check source and bytecode inspection without executing arithmetic or accepting extra arguments.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#-----------------------------------------------------------------------------

if(NOT DEFINED TOOL OR NOT DEFINED WORK)
    message(FATAL_ERROR "TOOL and WORK are required")
endif()
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/expression.uc" "int main(void) { return -(2 + 3 * 4); }\n")
file(WRITE "${WORK}/zero-divisor.uc" "int main(void) { return 1 / 0; }\n")
function(run_checked expected)
    execute_process(COMMAND "${TOOL}" ${ARGN}
        RESULT_VARIABLE code OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
    if(NOT "${code}" STREQUAL "${expected}")
        message(FATAL_ERROR "Unexpected exit ${code}: ${ARGN}\n${out}\n${err}")
    endif()
    string(REPLACE "\r\n" "\n" out "${out}")
    set(output "${out}" PARENT_SCOPE)
endfunction()
run_checked(0 inspect "${WORK}/expression.uc")
set(expected "Instructions: 7\nPeak stack values: 3\nConstants: 3\nArithmetic operations: 3\nEncoded bytes: 71\nStructural inspection only; arithmetic has not been executed.\n")
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "Unexpected inspection: ${output}")
endif()
# Exclusive compiler output is preserved across runs by choosing a fresh name.
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef suffix)
set(binary "${WORK}/expression-${suffix}.umcb")
run_checked(0 compile "${WORK}/expression.uc" "${binary}")
run_checked(0 inspect-bytecode "${binary}")
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "Source and bytecode inspection disagree: ${output}")
endif()
run_checked(0 inspect "${WORK}/zero-divisor.uc")
if(NOT output MATCHES "Structural inspection only")
    message(FATAL_ERROR "Inspection must explain its execution boundary")
endif()
run_checked(1 run "${WORK}/zero-divisor.uc")
run_checked(2 inspect "${WORK}/expression.uc" --max-steps 7)
run_checked(2 inspect-bytecode "${binary}" unexpected)
file(WRITE "${WORK}/invalid.umcb" "UMCB")
run_checked(1 inspect-bytecode "${WORK}/invalid.umcb")
if(output MATCHES "Instructions:")
    message(FATAL_ERROR "Malformed input published an inspection result")
endif()
