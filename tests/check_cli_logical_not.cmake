#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_logical_not.cmake
#
# PURPOSE:
#   Qualify logical negation through real source and bytecode CLI commands.
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
set(binary "${WORK}/logical-not-${suffix}.umcb")
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
    message(FATAL_ERROR "Negation lowering has an unexpected operation count: ${output}")
endif()
run_checked(0 compile "${SOURCE}" "${binary}")
file(READ "${binary}" prefix OFFSET 0 LIMIT 8 HEX)
if(NOT prefix STREQUAL "554d434202000400")
    message(FATAL_ERROR "Negation did not use the existing comparison wire profile: ${prefix}")
endif()
file(SHA256 "${binary}" binary_before)
foreach(mode IN ITEMS run exec)
    set(input "${SOURCE}")
    if(mode STREQUAL "exec")
        set(input "${binary}")
    endif()
    run_checked(0 "${mode}" "${input}" --max-steps 4)
    if(NOT output STREQUAL "Result: 1\nInstructions: 4\n")
        message(FATAL_ERROR "Unexpected negation result: ${output}")
    endif()
    run_checked(1 "${mode}" "${input}" --max-steps 3)
    if(output MATCHES "Result:" OR NOT error MATCHES "instruction budget exhausted")
        message(FATAL_ERROR "Exhausted negation published a result: ${output} ${error}")
    endif()
endforeach()
run_checked(0 trace "${SOURCE}")
set(source_trace "${output}")
if(NOT output MATCHES "opcode=equal stack-depth=1 top=1 status=success")
    message(FATAL_ERROR "Missing lowered equality trace: ${output}")
endif()
run_checked(0 trace-bytecode "${binary}")
if(NOT output STREQUAL source_trace)
    message(FATAL_ERROR "Source and wire negation traces differ")
endif()
run_checked(2 compile "${SOURCE}" "${binary}")
file(SHA256 "${SOURCE}" source_after)
file(SHA256 "${binary}" binary_after)
if(NOT source_before STREQUAL source_after OR NOT binary_before STREQUAL binary_after)
    message(FATAL_ERROR "The CLI changed an existing fixture or artifact")
endif()
