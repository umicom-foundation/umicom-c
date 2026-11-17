#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_source_regions.cmake
#
# PURPOSE:
#   Verify CLI region classification, malformed input refusal and rejection by the ordinary compiler.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------


file(SHA256 "${SOURCE}" before)
execute_process(COMMAND "${TOOL}" regions "${SOURCE}" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
if(NOT result EQUAL 0 OR NOT out MATCHES "language=basic" OR NOT out MATCHES "language=java"
   OR NOT out MATCHES "language=c23" OR NOT out MATCHES "Classification only")
    message(FATAL_ERROR "Missing language discovery: ${result} ${out} ${err}")
endif()
execute_process(COMMAND "${TOOL}" check "${SOURCE}" RESULT_VARIABLE checked OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
if(NOT checked EQUAL 1)
    message(FATAL_ERROR "Tagged file unexpectedly compiled: ${checked} ${out} ${err}")
endif()
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/bad.umc" "<?java int x;")
execute_process(COMMAND "${TOOL}" regions "${WORK}/bad.umc" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
if(NOT result EQUAL 1 OR NOT err MATCHES "missing its closing delimiter" OR out MATCHES "region=")
    message(FATAL_ERROR "Malformed source published regions: ${result} ${out} ${err}")
endif()
file(SHA256 "${SOURCE}" after)
if(NOT before STREQUAL after)
    message(FATAL_ERROR "Input source changed")
endif()
