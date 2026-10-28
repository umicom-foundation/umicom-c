#-----------------------------------------------------------------------------
# Umicom C
# File: examples/sdk_consumer/CMakeLists.txt
#
# PURPOSE:
#   Exercise installed package targets in a separate consumer.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.24)
project(UmicomCConsumer LANGUAGES C)
find_package(UmicomC 0.1 CONFIG REQUIRED)
add_executable(umicom-c-consumer main.c)
target_link_libraries(umicom-c-consumer PRIVATE UmicomC::Compiler)
include(CTest)
add_test(NAME umicom.c.installed_consumer COMMAND umicom-c-consumer)
set_tests_properties(umicom.c.installed_consumer PROPERTIES PASS_REGULAR_EXPRESSION "Result: 42")
