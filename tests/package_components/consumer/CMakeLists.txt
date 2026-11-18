#-----------------------------------------------------------------------------
# Umicom C
# File: tests/package_components/consumer/CMakeLists.txt
#
# PURPOSE:
#   Isolate the real package template from library installation and linkage.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.24)
project(UmicomCPackageComponentConsumer LANGUAGES NONE)
include(CMakePackageConfigHelpers)
if(NOT EXISTS "${TEMPLATE}" OR NOT DEFINED CASE)
    message(FATAL_ERROR "Provide the actual package template and a declared test case")
endif()
set(_package "${CMAKE_CURRENT_BINARY_DIR}/package")
file(MAKE_DIRECTORY "${_package}")
# Import-only stand-ins exercise CMake discovery semantics. They do not claim
# that archives were built, installed, linked, or compatible with this machine.
set(_targets "Compiler;Runtime;ManagedHeap")
if(CASE STREQUAL "missing")
    list(REMOVE_ITEM _targets ManagedHeap)
endif()
set(_imports "")
foreach(_target IN LISTS _targets)
    string(APPEND _imports "if(NOT TARGET UmicomC::${_target})\nadd_library(UmicomC::${_target} INTERFACE IMPORTED)\nendif()\n")
endforeach()
file(WRITE "${_package}/UmicomCTargets.cmake" "${_imports}")
configure_package_config_file("${TEMPLATE}" "${_package}/UmicomCConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/UmicomC")
set(UmicomC_DIR "${_package}")
if(CASE STREQUAL "all")
    find_package(UmicomC REQUIRED CONFIG COMPONENTS Compiler Runtime ManagedHeap)
elseif(CASE STREQUAL "optional")
    find_package(UmicomC REQUIRED CONFIG COMPONENTS Compiler OPTIONAL_COMPONENTS Missing)
    if(UmicomC_Missing_FOUND)
        message(FATAL_ERROR "An absent optional component was reported as present")
    endif()
elseif(CASE STREQUAL "unknown")
    find_package(UmicomC REQUIRED CONFIG COMPONENTS Missing)
elseif(CASE STREQUAL "case-sensitive")
    find_package(UmicomC REQUIRED CONFIG COMPONENTS compiler)
elseif(CASE STREQUAL "missing")
    find_package(UmicomC REQUIRED CONFIG COMPONENTS ManagedHeap)
elseif(CASE STREQUAL "repeat")
    find_package(UmicomC QUIET CONFIG COMPONENTS Missing)
    if(UmicomC_FOUND)
        message(FATAL_ERROR "Missing required component was accepted by quiet discovery")
    endif()
    find_package(UmicomC REQUIRED CONFIG COMPONENTS Runtime)
else()
    find_package(UmicomC REQUIRED CONFIG COMPONENTS "${CASE}")
endif()
if(NOT UmicomC_FOUND)
    message(FATAL_ERROR "A successful discovery did not set UmicomC_FOUND")
endif()
if(CASE STREQUAL "all")
    foreach(_component Compiler Runtime ManagedHeap)
        if(NOT UmicomC_${_component}_FOUND)
            message(FATAL_ERROR "A requested component has no positive result")
        endif()
    endforeach()
endif()
