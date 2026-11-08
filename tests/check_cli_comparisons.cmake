#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_comparisons.cmake
#
# PURPOSE:
#   Exercise comparison CLI commands, exact wire profiles, diagnostics and unchanged input artifacts.
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
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef suffix)
set(source "${WORK}/comparison-${suffix}.uc")
set(binary "${WORK}/comparison-${suffix}.umcb")
set(fault "${WORK}/eager-${suffix}.uc")
file(WRITE "${source}" "int main(void) { return 2 + 3 < 2 * 3; }
")
file(WRITE "${fault}" "int main(void) { return 0 == (1 / 0); }
")
function(run_checked expected)
    execute_process(COMMAND "${TOOL}" ${ARGN} RESULT_VARIABLE code
        OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
    if(NOT "${code}" STREQUAL "${expected}")
        message(FATAL_ERROR "Unexpected exit ${code}: ${ARGN}
${out}
${err}")
    endif()
    string(REPLACE "\r\n" "\n" out "${out}")
    set(output "${out}" PARENT_SCOPE)
    set(error "${err}" PARENT_SCOPE)
endfunction()
function(require_output fragment)
    string(FIND "${output}" "${fragment}" found)
    if(found LESS 0)
        message(FATAL_ERROR "Missing expected output ${fragment}: ${output}")
    endif()
endfunction()
run_checked(0 --version)
require_output("arithmetic and comparison profiles; bytecode formats 1 and 2")
run_checked(0 check "${source}")
require_output("accepted: 8 instructions")
run_checked(0 compile "${source}" "${binary}")
file(READ "${binary}" prefix OFFSET 0 LIMIT 8 HEX)
if(NOT prefix STREQUAL "554d434202000800")
    message(FATAL_ERROR "Comparison profile header differs: ${prefix}")
endif()
file(SHA256 "${source}" source_before)
file(SHA256 "${binary}" binary_before)
foreach(mode IN ITEMS run exec)
    if(mode STREQUAL "run")
        set(input "${source}")
    else()
        set(input "${binary}")
    endif()
    run_checked(0 "${mode}" "${input}" --max-steps 8)
    require_output("Result: 1
Instructions: 8
")
    run_checked(1 "${mode}" "${input}" --max-steps 7)
    if(output MATCHES "Result:" OR NOT error MATCHES "instruction budget exhausted")
        message(FATAL_ERROR "Exhausted comparison published a result: ${output} ${error}")
    endif()
endforeach()
run_checked(0 inspect "${source}")
require_output("Instructions: 8
Peak stack values: 3
Constants: 4
Arithmetic operations: 2
Encoded bytes: 80
")
set(source_inspection "${output}")
run_checked(0 inspect-bytecode "${binary}")
if(NOT output STREQUAL source_inspection)
    message(FATAL_ERROR "Wire inspection differs")
endif()
run_checked(0 disassemble "${binary}")
require_output("less")
run_checked(0 trace "${source}")
require_output("opcode=less stack-depth=1 top=1 status=success")
set(source_trace "${output}")
run_checked(0 trace-bytecode "${binary}")
if(NOT output STREQUAL source_trace)
    message(FATAL_ERROR "Comparison trace changed in transport")
endif()
run_checked(1 trace "${fault}")
if(NOT output MATCHES "opcode=divide" OR output MATCHES "opcode=equal" OR output MATCHES "Result:")
    message(FATAL_ERROR "Comparison skipped an operand fault: ${output} ${error}")
endif()
run_checked(2 compile "${source}" "${binary}")
file(SHA256 "${source}" source_after)
file(SHA256 "${binary}" binary_after)
if(NOT source_before STREQUAL source_after OR NOT binary_before STREQUAL binary_after)
    message(FATAL_ERROR "CLI changed an input or replaced an existing artifact")
endif()
