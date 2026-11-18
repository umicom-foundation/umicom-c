#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_execution_bounds.cmake
#
# PURPOSE:
#   Check additive read-only path-budget commands, source/wire parity and exact arity.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#-----------------------------------------------------------------------------
foreach(required TOOL SOURCE WORK)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
file(MAKE_DIRECTORY "${WORK}")
function(run_checked expected)
    execute_process(COMMAND "${TOOL}" ${ARGN} RESULT_VARIABLE code
        OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
    if(NOT "${code}" STREQUAL "${expected}")
        message(FATAL_ERROR "Unexpected exit ${code}: ${ARGN}
${out}
${err}")
    endif()
    # Preserve the draft literal for review; CMake needs explicit escaped CR/LF spelling.
    if(FALSE)
    string(REPLACE "
" "
" out "${out}")
    endif()
    string(REPLACE "\r\n" "\n" out "${out}")
    set(output "${out}" PARENT_SCOPE)
endfunction()
file(SHA256 "${SOURCE}" source_before)
run_checked(0 budget "${SOURCE}")
set(expected "Structural minimum instructions: 4
Structural maximum instructions: 7
Both branch alternatives are considered; arithmetic has not been executed.
")
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "Unexpected budget: ${output}")
endif()
string(RANDOM LENGTH 20 ALPHABET 0123456789abcdef suffix)
set(binary "${WORK}/budget-${suffix}.umcb")
run_checked(0 compile "${SOURCE}" "${binary}")
file(SHA256 "${binary}" binary_before)
run_checked(0 budget-bytecode "${binary}")
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "Source and bytecode budgets differ: ${output}")
endif()
run_checked(0 run "${SOURCE}" --max-steps 7)
if(NOT output MATCHES "Result: 42" OR NOT output MATCHES "Instructions: 7")
    message(FATAL_ERROR "Unexpected bounded execution: ${output}")
endif()
run_checked(1 run "${SOURCE}" --max-steps 6)
file(WRITE "${WORK}/fault.uc" "int main(void){return 1/0;}
")
run_checked(0 budget "${WORK}/fault.uc")
if(NOT output MATCHES "Structural minimum instructions: 4" OR NOT output MATCHES "Structural maximum instructions: 4")
    message(FATAL_ERROR "Arithmetic was executed or costs were misreported: ${output}")
endif()
run_checked(1 run "${WORK}/fault.uc")
run_checked(2 budget "${SOURCE}" --max-steps 7)
run_checked(2 budget-bytecode "${binary}" unexpected)
file(WRITE "${WORK}/invalid.umcb" "UMCB")
run_checked(1 budget-bytecode "${WORK}/invalid.umcb")
if(output MATCHES "Structural minimum")
    message(FATAL_ERROR "Invalid bytecode published a budget")
endif()
file(SHA256 "${SOURCE}" source_after)
file(SHA256 "${binary}" binary_after)
if(NOT source_before STREQUAL source_after OR NOT binary_before STREQUAL binary_after)
    message(FATAL_ERROR "Read-only budget commands modified input")
endif()
