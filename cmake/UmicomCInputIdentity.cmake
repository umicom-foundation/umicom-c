#-----------------------------------------------------------------------------
# Umicom C
# File: cmake/UmicomCInputIdentity.cmake
#
# PURPOSE:
#   Record the standalone compiler configuration inputs without claiming artifact provenance.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------

include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/InputIdentity.cmake")

# This adapter describes Umicom C's own CMake configuration. Runtime-only users
# keep their existing independent graph. Framework owns reusable test receipts;
# this record is neither a test-result cache nor permission to reuse binaries.
function(umicom_c_record_configuration_inputs)
    if(NOT PROJECT_IS_TOP_LEVEL)
        return()
    endif()
    set(folder "${PROJECT_BINARY_DIR}/qualification")
    file(MAKE_DIRECTORY "${folder}")
    set(recipe "{}")
    foreach(key IN ITEMS
        CMAKE_COMMAND CMAKE_VERSION CMAKE_GENERATOR CMAKE_GENERATOR_PLATFORM CMAKE_GENERATOR_TOOLSET
        CMAKE_BUILD_TYPE CMAKE_CONFIGURATION_TYPES CMAKE_C_STANDARD
        CMAKE_C_EXTENSIONS CMAKE_C_COMPILER CMAKE_C_COMPILER_ID
        CMAKE_C_COMPILER_VERSION CMAKE_C_COMPILER_TARGET CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN
        CMAKE_C_COMPILER_LAUNCHER CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL CMAKE_SYSROOT
        CMAKE_TOOLCHAIN_FILE CMAKE_MAKE_PROGRAM CMAKE_LINKER CMAKE_AR CMAKE_RANLIB
        CMAKE_EXE_LINKER_FLAGS CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_EXE_LINKER_FLAGS_RELEASE
        CMAKE_RC_COMPILER CMAKE_RC_FLAGS CMAKE_HOST_SYSTEM_NAME CMAKE_HOST_SYSTEM_VERSION
        CMAKE_HOST_SYSTEM_PROCESSOR CMAKE_SYSTEM_NAME CMAKE_SYSTEM_PROCESSOR
        UMICOM_C_BUILD_TOOLS UMICOM_C_BUILD_TESTS BUILD_TESTING)
        umc_input_json_quote(value "${${key}}")
        string(JSON recipe SET "${recipe}" "${key}" "${value}")
    endforeach()
    # A flag can contain @NAME@ literally. Preserve the former substituting
    # writer for review and use the data-only helper for the active recipe.
    if(FALSE)
        file(CONFIGURE OUTPUT "${folder}/configuration-recipe.json" CONTENT "${recipe}\n" @ONLY)
    endif()
    umc_input_write_data("${folder}/configuration-recipe.json" "${recipe}\n")
    umc_input_identity(capture "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}"
        "${folder}/configuration-recipe.json" "${folder}/configuration-inputs.json")
    add_custom_target(umicom-c-check-inputs
        COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE=${PROJECT_SOURCE_DIR}" "-DBINARY=${PROJECT_BINARY_DIR}"
            "-DRECIPE=${folder}/configuration-recipe.json"
            "-DRECEIPT=${folder}/configuration-inputs.json"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CheckInputIdentity.cmake"
        COMMENT "Checking configured inputs only; no binary or test success is implied"
        VERBATIM)
    if(UMICOM_C_BUILD_TESTS AND BUILD_TESTING)
        add_test(NAME umicom.c.inputs-current
            COMMAND "${CMAKE_COMMAND}"
                "-DSOURCE=${PROJECT_SOURCE_DIR}" "-DBINARY=${PROJECT_BINARY_DIR}"
                "-DRECIPE=${folder}/configuration-recipe.json"
                "-DRECEIPT=${folder}/configuration-inputs.json"
                -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CheckInputIdentity.cmake")
        set_tests_properties(umicom.c.inputs-current PROPERTIES
            TIMEOUT 60 LABELS "umicom-c;configuration-inputs")
    endif()
endfunction()
