#-----------------------------------------------------------------------------
# Umicom C
# File: CMakeLists.txt
#
# PURPOSE:
#   Build the small compiler and independently reusable runtime, with explicit tests and SDK exports.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.24)
project(UmicomC VERSION 0.1.0 LANGUAGES C)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(UMICOM_C_BUILD_TOOLS "Build the hosted command-line compiler" ON)
option(UMICOM_C_BUILD_TESTS "Build compiler and runtime regression coverage" ${PROJECT_IS_TOP_LEVEL})
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(cmake/UmicomCRuntime.cmake)
umicom_c_add_runtime(umicom_c_runtime)
add_library(UmicomC::Runtime ALIAS umicom_c_runtime)
set_target_properties(umicom_c_runtime PROPERTIES EXPORT_NAME Runtime)

add_library(umicom_c_compiler STATIC src/lexer.c src/compiler.c)
target_link_libraries(umicom_c_compiler PUBLIC umicom_c_runtime)
target_compile_features(umicom_c_compiler PUBLIC c_std_23)
add_library(UmicomC::Compiler ALIAS umicom_c_compiler)
set_target_properties(umicom_c_compiler PROPERTIES EXPORT_NAME Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom_c_compiler PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(UMICOM_C_BUILD_TOOLS)
    add_executable(umicom-c tools/main.c)
    target_link_libraries(umicom-c PRIVATE umicom_c_compiler)
    set_target_properties(umicom-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(umicom-c PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    install(TARGETS umicom-c RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

install(TARGETS umicom_c_runtime umicom_c_compiler EXPORT UmicomCTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY include/umicom_c DESTINATION include)
install(EXPORT UmicomCTargets NAMESPACE UmicomC::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/UmicomC")
configure_package_config_file(cmake/UmicomCConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/UmicomCConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/UmicomC")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/UmicomCConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/UmicomCConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/UmicomCConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/UmicomC")

if(UMICOM_C_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

# Installed consumers receive the same licence and language limits as source readers.
install(FILES LICENSE README.md DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/umicom-c")
install(DIRECTORY docs/ DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/umicom-c")
