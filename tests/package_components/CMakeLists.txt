#-----------------------------------------------------------------------------
# Umicom C
# File: tests/package_components/CMakeLists.txt
#
# PURPOSE:
#   Exercise actual package component admission with import-only configuration fixtures.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(UmicomCPackageComponents LANGUAGES NONE)
    include(CTest)
endif()
foreach(_case IN ITEMS Compiler Runtime ManagedHeap all optional unknown missing case-sensitive repeat)
    add_test(NAME umicom.c.package_components.${_case}
        COMMAND "${CMAKE_COMMAND}"
            "-DTEMPLATE=${CMAKE_CURRENT_LIST_DIR}/../../cmake/UmicomCConfig.cmake.in"
            "-DCASE=${_case}"
            "-DCONSUMER=${CMAKE_CURRENT_LIST_DIR}/consumer"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/cases/${_case}"
            -P "${CMAKE_CURRENT_LIST_DIR}/check.cmake")
    set_tests_properties(umicom.c.package_components.${_case} PROPERTIES
        TIMEOUT 45 LABELS "umicom-c;sdk;cmake;components")
endforeach()
