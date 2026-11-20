#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_int_input_cli.cmake
# PURPOSE: Qualify the required-input tooling against new and preserved files.
# AUTHOR AND ORGANISATION: Sammy Hegab / Umicom Foundation
# LICENCE: MIT
#-----------------------------------------------------------------------------
if(NOT EXISTS "${TOOL}" OR NOT EXISTS "${SOURCE}" OR NOT IS_ABSOLUTE "${WORK}")
    message(FATAL_ERROR "Select the built Umicom C tool, checked source and private working path.")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef suffix)
set(work "${WORK}/${suffix}")
file(MAKE_DIRECTORY "${work}")
set(artifact "${work}/answer.umcb")

function(check expected output error)
    execute_process(COMMAND "${TOOL}" ${ARGN} RESULT_VARIABLE status
        OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr TIMEOUT 10)
    if(NOT "${status}" STREQUAL "${expected}" OR
        (NOT "${output}" STREQUAL "" AND NOT stdout MATCHES "${output}") OR
        (NOT "${error}" STREQUAL "" AND NOT stderr MATCHES "${error}"))
        message(FATAL_ERROR "Expected command result ${expected}. Observed ${status}\n${stdout}${stderr}")
    endif()
endfunction()

check(0 "Compiled a required-input artifact" "" compile-input "${SOURCE}" "${artifact}")
file(SHA256 "${artifact}" checksum)
check(0 "Result: 42" "" exec-input "${artifact}" +20)
check(0 "Result: -10" "" exec-input "${artifact}" -5)
check(1 "" "invalid bytecode structure" exec "${artifact}")
check(1 "" "instruction budget exhausted" exec-input "${artifact}" 20 --max-steps 1)
check(2 "" "requires a decimal integer" exec-input "${artifact}" 20 --max-steps 0)
check(2 "" "signed 32-bit decimal integer" exec-input "${work}/absent.umcb" 2147483648)
check(2 "" "signed 32-bit decimal integer" exec-input "${work}/absent.umcb" -2147483649)
check(2 "" "existing files are preserved" compile-input "${SOURCE}" "${artifact}")
file(SHA256 "${artifact}" after)
if(NOT checksum STREQUAL after)
    message(FATAL_ERROR "Binding or refusal changed the original artifact.")
endif()
