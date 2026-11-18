#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_basic_block_regions.cmake
#
# PURPOSE:
#   Check real CLI discovery and malformed BASIC block refusal without executing foreign code.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------
if(NOT EXISTS "${TOOL}" OR IS_DIRECTORY "${TOOL}")
    message(FATAL_ERROR "Build the Umicom C CLI before this check")
endif()
file(SHA256 "${SOURCE}" before)
execute_process(COMMAND "${TOOL}" regions "${SOURCE}" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
string(REGEX MATCHALL "language=basic" basic_rows "${out}")
list(LENGTH basic_rows basic_count)
if(NOT result EQUAL 0 OR NOT basic_count EQUAL 1 OR NOT out MATCHES "Classification only")
    message(FATAL_ERROR "Nested comment discovery failed: ${result} ${out} ${err}")
endif()
execute_process(COMMAND "${TOOL}" check "${SOURCE}" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
if(NOT result EQUAL 1)
    message(FATAL_ERROR "Envelope incorrectly accepted by integer compiler: ${result} ${out} ${err}")
endif()
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/unterminated.umc" "<?basic\n/' outer /' nested '/ ?>")
execute_process(COMMAND "${TOOL}" regions "${WORK}/unterminated.umc" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
if(NOT result EQUAL 1 OR NOT err MATCHES "Unterminated quoted text or comment" OR out MATCHES "region=")
    message(FATAL_ERROR "Malformed block published region results: ${result} ${out} ${err}")
endif()
file(SHA256 "${SOURCE}" after)
if(NOT before STREQUAL after)
    message(FATAL_ERROR "CLI modified the source example")
endif()
