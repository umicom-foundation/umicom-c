#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_trace.cmake
#
# PURPOSE:
#   Exercise trace commands, diagnostic failures, exact arguments and unchanged inputs.
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
set(source "${WORK}/trace-${suffix}.uc")
set(binary "${WORK}/trace-${suffix}.umcb")
set(fault "${WORK}/fault-${suffix}.uc")
set(invalid "${WORK}/invalid-${suffix}.uc")
file(WRITE "${source}" "int main(void) { return 6 * 7; }\n")
file(WRITE "${fault}" "int main(void) { return 7 / 0; }\n")
file(WRITE "${invalid}" "int main(void) { return ; }\n")
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
run_checked(0 compile "${source}" "${binary}")
file(SHA256 "${source}" source_before)
file(SHA256 "${binary}" binary_before)
run_checked(0 trace "${source}" --max-steps 4)
set(source_trace "${output}")
foreach(fragment IN ITEMS "opcode=push stack-depth=1 top=6 status=success"
    "opcode=push stack-depth=2 top=7 status=success"
    "opcode=multiply stack-depth=1 top=42 status=success"
    "opcode=return stack-depth=1 top=42 status=success" "Result: 42\nInstructions: 4\n")
    string(FIND "${output}" "${fragment}" found)
    if(found LESS 0)
        message(FATAL_ERROR "Trace lacks expected arithmetic state: ${fragment}\n${output}")
    endif()
endforeach()
run_checked(0 trace-bytecode "${binary}" --max-steps 4)
if(NOT output STREQUAL source_trace)
    message(FATAL_ERROR "Wire round trip changed the trace")
endif()
foreach(mode IN ITEMS trace trace-bytecode)
    if(mode STREQUAL "trace")
        set(input "${source}")
    else()
        set(input "${binary}")
    endif()
    run_checked(1 "${mode}" "${input}" --max-steps 3)
    if(output MATCHES "Result:" OR NOT output MATCHES "opcode=return stack-depth=1 top=42 status=instruction budget exhausted"
            OR error STREQUAL "")
        message(FATAL_ERROR "Exhaustion did not retain the rejected RETURN without a result: ${output}: ${error}")
    endif()
    run_checked(0 "${mode}" "${input}")
    foreach(value IN ITEMS 0 -1 +4 257 4x)
        run_checked(2 "${mode}" "${input}" --max-steps "${value}")
        if(output MATCHES "step=" OR output MATCHES "Result:")
            message(FATAL_ERROR "Invalid budget executed instructions")
        endif()
    endforeach()
    run_checked(2 "${mode}" "${input}" --max-steps 4 unexpected)
    run_checked(2 "${mode}" "${input}" --max-steps)
    run_checked(2 "${mode}" "${input}" --unknown 4)
endforeach()
run_checked(1 trace "${fault}")
if(NOT output MATCHES "opcode=divide stack-depth=2 top=0 status=division by zero or signed arithmetic overflow"
        OR output MATCHES "opcode=return" OR output MATCHES "Result:" OR error STREQUAL "")
    message(FATAL_ERROR "Arithmetic fault lost its unmodified input stack: ${output}: ${error}")
endif()
run_checked(1 trace "${invalid}")
if(output MATCHES "step=" OR output MATCHES "Result:")
    message(FATAL_ERROR "Invalid source produced an execution trace")
endif()
file(WRITE "${WORK}/invalid-${suffix}.umcb" "UMCB")
run_checked(1 trace-bytecode "${WORK}/invalid-${suffix}.umcb")
if(output MATCHES "step=" OR output MATCHES "Result:")
    message(FATAL_ERROR "Invalid bytecode produced an execution trace")
endif()
file(SHA256 "${source}" source_after)
file(SHA256 "${binary}" binary_after)
if(NOT source_before STREQUAL source_after OR NOT binary_before STREQUAL binary_after)
    message(FATAL_ERROR "Tracing modified its input")
endif()
