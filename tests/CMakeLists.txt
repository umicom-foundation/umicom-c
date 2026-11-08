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
    add_test(NAME umicom.c.cli-budget
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/budget-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_budget.cmake")
    set_tests_properties(umicom.c.cli-budget PROPERTIES LABELS "umicom-c;cli" TIMEOUT 30)
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

# Inspection and execution share validation but have different success claims.
add_executable(umicom-c-test-inspection test_inspection.c)
target_link_libraries(umicom-c-test-inspection PRIVATE UmicomC::Runtime)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-inspection PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
add_test(NAME umicom.c.inspection COMMAND umicom-c-test-inspection)
set_tests_properties(umicom.c.inspection PROPERTIES LABELS "umicom-c;unit;inspection" TIMEOUT 10)
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-inspection
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/inspection-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_inspection.cmake")
    set_tests_properties(umicom.c.cli-inspection PROPERTIES LABELS "umicom-c;cli;inspection" TIMEOUT 30)
endif()

# Trace cases exercise the same runtime linked by hosted and freestanding clients.
add_executable(umicom-c-test-trace test_trace.c)
target_link_libraries(umicom-c-test-trace PRIVATE UmicomC::Runtime)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-trace PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
foreach(case IN ITEMS
    success fuel-boundaries add-overflow subtract-overflow multiply-overflow divide-zero divide-overflow negative-division minimum-add maximum-subtract unary short-capacity zero-capacity bad-opcode reserved-operand bad-source-offset early-return missing-return null-program zero-count excess-count null-result null-storage null-count reuse wire max-program max-depth stack-overflow)
    add_test(NAME "umicom.c.trace.${case}" COMMAND umicom-c-test-trace "${case}")
    set_tests_properties("umicom.c.trace.${case}" PROPERTIES LABELS "umicom-c;unit;trace" TIMEOUT 10)
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-trace
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/trace-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_trace.cmake")
    set_tests_properties(umicom.c.cli-trace PROPERTIES LABELS "umicom-c;cli;trace" TIMEOUT 60)
endif()

# Comparison source, runtime and transport cases remain separate evidence layers.
add_executable(umicom-c-test-comparison-source test_comparison_source.c)
target_link_libraries(umicom-c-test-comparison-source PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-comparison-source PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
foreach(case IN ITEMS less less-false less-equal greater greater-equal equal not-equal negative minimum maximum additive-first relational-first equality-left relational-left parentheses unary-result nested comment-boundary both-operands bare-assignment bare-not spaced-operator missing-right logical-and)
    add_test(NAME "umicom.c.comparison.source.${case}" COMMAND umicom-c-test-comparison-source "${case}")
    set_tests_properties("umicom.c.comparison.source.${case}" PROPERTIES LABELS "umicom-c;comparison;source" TIMEOUT 10)
endforeach()
add_executable(umicom-c-test-comparison-runtime test_comparison_runtime.c)
target_link_libraries(umicom-c-test-comparison-runtime PRIVATE UmicomC::Runtime)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-comparison-runtime PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
foreach(case IN ITEMS truth-matrix extreme-values fuel-boundaries trace underflow reserved-operand inspection short-trace last-opcode unknown-opcode)
    add_test(NAME "umicom.c.comparison.runtime.${case}" COMMAND umicom-c-test-comparison-runtime "${case}")
    set_tests_properties("umicom.c.comparison.runtime.${case}" PROPERTIES LABELS "umicom-c;comparison;runtime" TIMEOUT 10)
endforeach()
add_executable(umicom-c-test-comparison-wire test_comparison_wire.c)
target_link_libraries(umicom-c-test-comparison-wire PRIVATE UmicomC::Runtime)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-comparison-wire PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
foreach(case IN ITEMS legacy-stable comparison-roundtrip legacy-mislabel comparison-mislabel unknown-version reserved-flags unknown-opcode truncated trailing small-capacity)
    add_test(NAME "umicom.c.comparison.wire.${case}" COMMAND umicom-c-test-comparison-wire "${case}")
    set_tests_properties("umicom.c.comparison.wire.${case}" PROPERTIES LABELS "umicom-c;comparison;wire" TIMEOUT 10)
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-comparisons
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/comparison-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_comparisons.cmake")
    set_tests_properties(umicom.c.cli-comparisons PROPERTIES LABELS "umicom-c;cli;comparison" TIMEOUT 60)
endif()
