#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_boolean_literals.cmake
#
# PURPOSE:
#   Verify Boolean source execution, wire traces, keyword refusal and artifact preservation.
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
set(binary "${WORK}/boolean-literals-${suffix}.umcb")
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

# The generated draft closed variable braces before the name and passed literal
# SOURCE}/binary} strings. Retain it for review; the active wrapper below uses
# complete CMake variable references and exercises the actual supplied files.
if(FALSE)
if(NOT output MATCHES "accepted: [0-9]+ instructions")
    message(FATAL_ERROR "Boolean example was not checked: ${}output}")
endif()
run_checked(0 compile "${}SOURCE}" "${}binary}")
file(READ "${}binary}" prefix OFFSET 0 LIMIT 5 HEX)
if(NOT prefix STREQUAL "554d434204")
    message(FATAL_ERROR "Expected existing forward-branch format 4: ${}prefix}")
endif()
endif()
if(NOT output MATCHES "accepted: [0-9]+ instructions")
    message(FATAL_ERROR "Boolean example was not checked: ${output}")
endif()
run_checked(0 compile "${SOURCE}" "${binary}")
file(READ "${binary}" prefix OFFSET 0 LIMIT 5 HEX)
if(NOT prefix STREQUAL "554d434204")
    message(FATAL_ERROR "Expected existing forward-branch format 4: ${prefix}")
endif()
file(SHA256 "${binary}" binary_before)
foreach(mode IN ITEMS run exec)
    set(input "${SOURCE}")
    if(mode STREQUAL "exec")
        set(input "${binary}")
    endif()
    run_checked(0 "${mode}" "${input}" --max-steps 256)
    if(NOT output MATCHES "^Result: 42\nInstructions: [0-9]+\n$")
        message(FATAL_ERROR "Source/wire result mismatch: ${output}")
    endif()
    run_checked(1 "${mode}" "${input}" --max-steps 1)
    if(output MATCHES "Result:" OR NOT error MATCHES "instruction budget exhausted")
        message(FATAL_ERROR "Fuel exhaustion published a result: ${output} ${error}")
    endif()
endforeach()
run_checked(0 trace "${SOURCE}")
set(source_trace "${output}")
run_checked(0 trace-bytecode "${binary}")
if(NOT output STREQUAL source_trace)
    message(FATAL_ERROR "Source offsets changed after wire roundtrip")
endif()
set(invalid "${WORK}/invalid-${suffix}.uc")
set(rejected "${WORK}/invalid-${suffix}.umcb")
file(WRITE "${invalid}" "int main(void){int true=1;return false;}")
run_checked(1 check "${invalid}")
# Declaration admission emits the existing specific name diagnostic. Preserve
# the generic draft expectation for review; do not weaken the production error.
if(FALSE)
    if(NOT error MATCHES "syntax error")
        message(FATAL_ERROR "Missing reserved-keyword diagnostic")
    endif()
endif()
if(NOT error MATCHES "expected a local integer name")
    message(FATAL_ERROR "Missing reserved-keyword diagnostic: ${error}")
endif()
run_checked(1 compile "${invalid}" "${rejected}")
if(EXISTS "${rejected}")
    message(FATAL_ERROR "Rejected source published bytecode")
endif()
run_checked(2 compile "${SOURCE}" "${binary}")
file(SHA256 "${SOURCE}" source_after)
file(SHA256 "${binary}" binary_after)
if(NOT source_before STREQUAL source_after OR NOT binary_before STREQUAL binary_after)
    message(FATAL_ERROR "Existing source/artifact changed")
endif()
