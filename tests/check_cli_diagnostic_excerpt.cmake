#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_cli_diagnostic_excerpt.cmake
#
# PURPOSE:
#   Verify escaped source context on CLI errors and warnings without exposing binary bytes.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#-----------------------------------------------------------------------------
foreach(required TOOL WORK)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
file(MAKE_DIRECTORY "${WORK}")
function(run_checked expected)
    execute_process(COMMAND "${TOOL}" ${ARGN} RESULT_VARIABLE code
        OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
    if(NOT "${code}" STREQUAL "${expected}")
        message(FATAL_ERROR "Unexpected exit ${code}: ${out} ${err}")
    endif()
    string(REPLACE "\r\n" "\n" err "${err}")
    set(errors "${err}" PARENT_SCOPE)
endfunction()
file(WRITE "${WORK}/divide.uc" "int main(void){\n\treturn 1/0;\n}\n")
file(SHA256 "${WORK}/divide.uc" before)
run_checked(1 run "${WORK}/divide.uc")
string(FIND "${errors}" "  | \\treturn 1/0;\n  |           ^\n" found)
if(found LESS 0)
    message(FATAL_ERROR "Escaped tab/caret missing: ${errors}")
endif()
file(SHA256 "${WORK}/divide.uc" after)
if(NOT before STREQUAL after)
    message(FATAL_ERROR "Diagnostic display changed source")
endif()
string(ASCII 27 escape)
file(WRITE "${WORK}/control.uc" "int main(void){return ${escape};}")
run_checked(1 check "${WORK}/control.uc")
string(FIND "${errors}" "\\x1B" escaped)
string(FIND "${errors}" "${escape}" raw)
if(escaped LESS 0 OR NOT raw EQUAL -1)
    message(FATAL_ERROR "Control byte was not rendered safely")
endif()
run_checked(2 check "${WORK}/control.uc" --warnings)
if(NOT errors MATCHES "[|]")
    message(FATAL_ERROR "Warning-check syntax error lacks excerpt")
endif()
run_checked(2 compare-operations "${WORK}/control.uc" "${WORK}/divide.uc")
if(NOT errors MATCHES "[|]")
    message(FATAL_ERROR "Comparison source error lacks excerpt")
endif()
file(WRITE "${WORK}/warning.uc" "int main(void){int unused=1;return 42;}")
run_checked(0 check "${WORK}/warning.uc" --warnings)
if(NOT errors MATCHES "warning:" OR NOT errors MATCHES "[|]")
    message(FATAL_ERROR "Unused-local warning lacks source context: ${errors}")
endif()
file(WRITE "${WORK}/broken.umcb" "UMCB")
# The draft used an unsupported alias. Keep it disabled; exec is the existing
# bytecode execution command and reaches the diagnostic path under test.
if(FALSE)
    run_checked(1 run-bytecode "${WORK}/broken.umcb")
endif()
run_checked(1 exec "${WORK}/broken.umcb")
if(errors MATCHES "[|]")
    message(FATAL_ERROR "Binary bytes were displayed as source")
endif()
