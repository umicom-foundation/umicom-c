#-----------------------------------------------------------------------------
# Umicom C
# File: cmake/UmicomCRuntime.cmake
#
# PURPOSE:
#   Let freestanding consumers build the same runtime without the compiler or hosted CLI.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------
include_guard(GLOBAL)
function(umicom_c_add_runtime target)
    if(TARGET "${target}")
        message(FATAL_ERROR "Runtime target already exists: ${target}")
    endif()
    get_filename_component(runtime_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
    add_library("${target}" STATIC
        "${runtime_root}/src/status.c"
        "${runtime_root}/src/vm.c"
        "${runtime_root}/src/bytecode.c")
    target_sources("${target}" PRIVATE "${runtime_root}/src/comparison.c")
    target_include_directories("${target}" PUBLIC
        "$<BUILD_INTERFACE:${runtime_root}/include>"
        "$<INSTALL_INTERFACE:include>")
    target_compile_features("${target}" PUBLIC c_std_23)
    set_target_properties("${target}" PROPERTIES C_EXTENSIONS OFF)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options("${target}" PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
endfunction()
