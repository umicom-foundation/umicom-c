#-----------------------------------------------------------------------------
# Umicom C
# File: tests/CMakeLists.txt
#
# PURPOSE:
#   Register independent regression groups without weakening failure diagnostics.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------
foreach(group IN ITEMS compiler vm bytecode)
    add_executable("umicom-c-test-${group}" "test_${group}.c")
    target_link_libraries("umicom-c-test-${group}" PRIVATE UmicomC::Compiler)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options("umicom-c-test-${group}" PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    add_test(NAME "umicom.c.${group}" COMMAND "umicom-c-test-${group}")
    set_tests_properties("umicom.c.${group}" PROPERTIES LABELS "umicom-c;unit")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/cli-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli.cmake")
    set_tests_properties(umicom.c.cli PROPERTIES LABELS "umicom-c;cli" TIMEOUT 30)
endif()

# Instruction identity remains independent of comment and whitespace locations.
add_executable(umicom-c-test-comparison test_comparison.c)
target_link_libraries(umicom-c-test-comparison PRIVATE UmicomC::Compiler)
add_test(NAME umicom.c.comparison COMMAND umicom-c-test-comparison)
set_tests_properties(umicom.c.comparison PROPERTIES LABELS "umicom-c;unit" TIMEOUT 10)

add_executable(umicom-c-test-roundtrip-corpus test_roundtrip_corpus.c)
target_link_libraries(umicom-c-test-roundtrip-corpus PRIVATE UmicomC::Compiler)
add_test(NAME umicom.c.roundtrip_corpus COMMAND umicom-c-test-roundtrip-corpus)
set_tests_properties(umicom.c.roundtrip_corpus PROPERTIES LABELS "umicom-c;unit" TIMEOUT 15)

# Fixed transport fixtures are independent of the parser and compiler library.
add_executable(umicom-c-test-wire-contract test_wire_contract.c)
target_link_libraries(umicom-c-test-wire-contract PRIVATE UmicomC::Runtime)
add_test(NAME umicom.c.wire_contract COMMAND umicom-c-test-wire-contract)
set_tests_properties(umicom.c.wire_contract PROPERTIES LABELS "umicom-c;unit;portability" TIMEOUT 10)
foreach(target IN ITEMS umicom-c-test-comparison umicom-c-test-roundtrip-corpus umicom-c-test-wire-contract)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
endforeach()
