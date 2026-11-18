#-----------------------------------------------------------------------------
# Umicom C
# File: tests/package_components/check.cmake
#
# PURPOSE:
#   Distinguish expected component rejection from a successful package configuration.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#-----------------------------------------------------------------------------

if(NOT DEFINED TEMPLATE OR NOT DEFINED CASE OR NOT DEFINED CONSUMER OR NOT DEFINED WORK)
    message(FATAL_ERROR "Package check requires template, case, consumer and isolated work paths")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CONSUMER}" -B "${WORK}"
    "-DTEMPLATE=${TEMPLATE}" "-DCASE=${CASE}"
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
file(WRITE "${WORK}/configuration-output.log" "${_out}\n${_err}")
if(CASE MATCHES "^(unknown|missing|case-sensitive)$")
    if(_result EQUAL 0 OR NOT "${_out}${_err}" MATCHES "UmicomC_FOUND")
        message(FATAL_ERROR "Expected required-component refusal, observed ${_result}:\n${_out}\n${_err}")
    endif()
elseif(NOT _result EQUAL 0)
    message(FATAL_ERROR "Expected component admission, observed ${_result}:\n${_out}\n${_err}")
endif()
message(STATUS "Package component case ${CASE} behaved as declared")
