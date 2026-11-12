#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_budget.cmake
#
# PURPOSE:
#   Check real source and bytecode command execution at the instruction limit.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------
if(NOT DEFINED TOOL OR NOT DEFINED WORK)
    message(FATAL_ERROR "The test requires TOOL and WORK paths")
endif()

# This regression starts 28 CLI processes in sequence. Bound each invocation
# independently so a hung child reports its failing command; the CTest outer
# allowance also includes startup/scheduling time on a loaded host. Neither
# wall-clock limit changes the VM instruction budgets exercised below.
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/source.uc" "int main(void) { return 6 * 7; }\n")
# Only this fixture's previous generated artifact is replaced on a rerun.
file(REMOVE "${WORK}/program.umcb")
execute_process(COMMAND "${TOOL}" compile "${WORK}/source.uc" "${WORK}/program.umcb"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "Fixture compilation failed: ${result}: ${output}: ${error}")
endif()
file(SHA256 "${WORK}/source.uc" source_before)
file(SHA256 "${WORK}/program.umcb" binary_before)
foreach(mode IN ITEMS run exec)
    if(mode STREQUAL "run")
        set(input "${WORK}/source.uc")
    else()
        set(input "${WORK}/program.umcb")
    endif()
    foreach(budget IN ITEMS 4 256 0004)
        execute_process(COMMAND "${TOOL}" "${mode}" "${input}" --max-steps "${budget}"
            RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
        if(NOT "${result}" STREQUAL "0" OR NOT output MATCHES "Result: 42"
                OR NOT output MATCHES "Instructions: 4")
            message(FATAL_ERROR "Exact or larger budget failed: ${mode}/${budget}: ${result}: ${output}: ${error}")
        endif()
    endforeach()
    execute_process(COMMAND "${TOOL}" "${mode}" "${input}" --max-steps 3
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
    if(NOT "${result}" STREQUAL "1" OR output MATCHES "Result:" OR error STREQUAL "")
        message(FATAL_ERROR "Exhaustion must fail without publishing a result: ${mode}: ${result}: ${output}: ${error}")
    endif()
    foreach(budget IN ITEMS 0 -1 +4 257 999999999999999999999999 4x "4 0" "")
        execute_process(COMMAND "${TOOL}" "${mode}" "${input}" --max-steps "${budget}"
            RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
        if(NOT "${result}" STREQUAL "2" OR output MATCHES "Result:")
            message(FATAL_ERROR "Invalid budget was accepted: ${mode}/${budget}: ${result}: ${output}: ${error}")
        endif()
    endforeach()
    execute_process(COMMAND "${TOOL}" "${mode}" "${input}" --max-steps 4 --max-steps 3
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
    if(NOT "${result}" STREQUAL "2")
        message(FATAL_ERROR "Duplicate options must be rejected")
    endif()
endforeach()
execute_process(COMMAND "${TOOL}" check "${WORK}/source.uc" --max-steps 4
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
if(NOT "${result}" STREQUAL "2")
    message(FATAL_ERROR "The check command must not silently accept execution options")
endif()
file(SHA256 "${WORK}/source.uc" source_after)
file(SHA256 "${WORK}/program.umcb" binary_after)
if(NOT source_before STREQUAL source_after OR NOT binary_before STREQUAL binary_after)
    message(FATAL_ERROR "Execution modified an input artifact")
endif()
