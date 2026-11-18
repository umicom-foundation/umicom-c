#-----------------------------------------------------------------------------
# Umicom C
# File: tests/check_d_regions.cmake
#
# PURPOSE:
#   Verify CLI D classification and explicit unsupported-string refusal without foreign execution.
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
string(REGEX MATCHALL "language=d " d_rows "${out}")
list(LENGTH d_rows d_count)
if(NOT result EQUAL 0 OR NOT d_count EQUAL 1 OR NOT out MATCHES "Classification only")
    message(FATAL_ERROR "D discovery failed: ${result} ${out} ${err}")
endif()
execute_process(COMMAND "${TOOL}" check "${SOURCE}" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
if(NOT result EQUAL 1)
    message(FATAL_ERROR "D envelope incorrectly accepted by integer compiler: ${result} ${out} ${err}")
endif()
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/unsupported.umc" [=[<?d q{ ?> } ?>]=])
execute_process(COMMAND "${TOOL}" regions "${WORK}/unsupported.umc" RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 10)
if(NOT result EQUAL 1 OR NOT err MATCHES "Unsupported D string form" OR out MATCHES "region=")
    message(FATAL_ERROR "Unsupported string published region results: ${result} ${out} ${err}")
endif()
file(SHA256 "${SOURCE}" after)
if(NOT before STREQUAL after)
    message(FATAL_ERROR "CLI modified the source example")
endif()
