#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_integer_literals.cmake
#
# PURPOSE:
#   Exercise source, wire, budget and failed compilation through the real CLI.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------

if(NOT DEFINED TOOL OR NOT DEFINED WORK OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "TOOL, WORK and SOURCE are required")
endif()
file(MAKE_DIRECTORY "${WORK}")
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef suffix)
set(binary "${WORK}/integer-literals-${suffix}.umcb")
file(SHA256 "${SOURCE}" source_before)
function(run_checked expected)
    execute_process(COMMAND "${TOOL}" ${ARGN} RESULT_VARIABLE code
        OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
    if(NOT "${code}" STREQUAL "${expected}")
        message(FATAL_ERROR "Unexpected exit ${code}: ${ARGN}\n${out}\n${err}")
    endif()
    string(REPLACE "\r\n" "\n" out "${out}")
    set(output "${out}" PARENT_SCOPE)
    set(error "${err}" PARENT_SCOPE)
endfunction()
run_checked(0 check "${SOURCE}")
if(NOT output MATCHES "accepted: 4 instructions")
    message(FATAL_ERROR "Numeric spelling changed instruction cost: ${output}")
endif()
run_checked(0 compile "${SOURCE}" "${binary}")
file(READ "${binary}" prefix OFFSET 0 LIMIT 8 HEX)
if(NOT prefix STREQUAL "554d434202000400")
    message(FATAL_ERROR "Comparison must retain format 2: ${prefix}")
endif()
file(SHA256 "${binary}" binary_before)
foreach(mode IN ITEMS run exec)
    set(input "${SOURCE}")
    if(mode STREQUAL "exec")
        set(input "${binary}")
    endif()
    run_checked(0 "${mode}" "${input}" --max-steps 4)
    if(NOT output STREQUAL "Result: 1\nInstructions: 4\n")
        message(FATAL_ERROR "Numeric source/wire execution disagrees: ${output}")
    endif()
    run_checked(1 "${mode}" "${input}" --max-steps 3)
    if(output MATCHES "Result:" OR NOT error MATCHES "instruction budget exhausted")
        message(FATAL_ERROR "Exhaustion published a value: ${output} ${error}")
    endif()
endforeach()
run_checked(0 trace "${SOURCE}")
set(source_trace "${output}")
run_checked(0 trace-bytecode "${binary}")
if(NOT output STREQUAL source_trace)
    message(FATAL_ERROR "Source locations differ after numeric wire roundtrip")
endif()
# Invalid source must not publish even a partial output artifact.
set(invalid "${WORK}/invalid-${suffix}.uc")
set(rejected "${WORK}/invalid-${suffix}.umcb")
file(WRITE "${invalid}" "int main(void){return 0x80000000;}")
run_checked(1 compile "${invalid}" "${rejected}")
if(EXISTS "${rejected}" OR output MATCHES "Result:")
    message(FATAL_ERROR "Failed literal admission published an artifact")
endif()
run_checked(2 compile "${SOURCE}" "${binary}")
file(SHA256 "${SOURCE}" source_after)
file(SHA256 "${binary}" binary_after)
if(NOT source_before STREQUAL source_after OR NOT binary_before STREQUAL binary_after)
    message(FATAL_ERROR "An existing input or artifact was overwritten")
endif()
