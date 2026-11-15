#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_complement.cmake
#
# PURPOSE:
#   Exercise signed complement through source, format-1 bytecode, trace and exact fuel.
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
set(binary "${WORK}/complement-${suffix}.umcb")
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
if(NOT output MATCHES "accepted: 5 instructions")
    message(FATAL_ERROR "Complement lowering changed instruction cost: ${output}")
endif()
run_checked(0 compile "${SOURCE}" "${binary}")
file(READ "${binary}" prefix OFFSET 0 LIMIT 8 HEX)
if(NOT prefix STREQUAL "554d434201000500")
    message(FATAL_ERROR "Complement updates must use canonical format 1: ${prefix}")
endif()
file(SHA256 "${binary}" binary_before)
foreach(mode IN ITEMS run exec)
    set(input "${SOURCE}")
    if(mode STREQUAL "exec")
        set(input "${binary}")
    endif()
    run_checked(0 "${mode}" "${input}" --max-steps 5)
    if(NOT output STREQUAL "Result: 42\nInstructions: 5\n")
        message(FATAL_ERROR "Complement source/wire execution disagrees: ${output}")
    endif()
    run_checked(1 "${mode}" "${input}" --max-steps 4)
    if(output MATCHES "Result:" OR NOT error MATCHES "instruction budget exhausted")
        message(FATAL_ERROR "Exhaustion published a value: ${output} ${error}")
    endif()
endforeach()
run_checked(0 trace "${SOURCE}")
set(source_trace "${output}")
run_checked(0 trace-bytecode "${binary}")
if(NOT output STREQUAL source_trace)
    message(FATAL_ERROR "Source locations differ after complement wire roundtrip")
endif()
# Invalid source must not publish even a partial output artifact.
set(invalid "${WORK}/invalid-${suffix}.uc")
set(rejected "${WORK}/invalid-${suffix}.umcb")
file(WRITE "${invalid}" "int main(void){int value=41;value~=1;return value;}")
run_checked(1 compile "${invalid}" "${rejected}")
if(EXISTS "${rejected}" OR output MATCHES "Result:")
    message(FATAL_ERROR "Failed complement admission published an artifact")
endif()
run_checked(2 compile "${SOURCE}" "${binary}")
file(SHA256 "${SOURCE}" source_after)
file(SHA256 "${binary}" binary_after)
if(NOT source_before STREQUAL source_after OR NOT binary_before STREQUAL binary_after)
    message(FATAL_ERROR "An existing input or artifact was overwritten")
endif()

run_checked(0 disassemble "${binary}")
if(NOT output MATCHES "push[ ]+-1" OR NOT output MATCHES "subtract")
    message(FATAL_ERROR "Complement must use existing arithmetic records: ${output}")
endif()
