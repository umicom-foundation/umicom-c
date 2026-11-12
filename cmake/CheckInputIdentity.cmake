#-----------------------------------------------------------------------------
# Umicom C
# File: cmake/CheckInputIdentity.cmake
#
# PURPOSE:
#   Check the current declared inputs against a configuration record without refreshing it.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.24)
foreach(argument IN ITEMS SOURCE BINARY RECIPE RECEIPT)
    if(NOT DEFINED ${argument} OR "${${argument}}" STREQUAL "")
        message(FATAL_ERROR "Required identity argument: ${argument}")
    endif()
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/InputIdentity.cmake")
umc_input_identity(verify "${SOURCE}" "${BINARY}" "${RECIPE}" "${RECEIPT}")
message(STATUS "Configured inputs match. Compilation, binary provenance and test outcomes remain separate.")
