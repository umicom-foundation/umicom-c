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
    # Preserve the earlier aggregate limit for review. Twenty-eight process
    # starts can exceed 30 seconds on a loaded Windows host. Child invocations
    # now have their own ten-second timeout; keep an outer scheduling bound.
    if(FALSE)
    set_tests_properties(umicom.c.cli-budget PROPERTIES LABELS "umicom-c;cli" TIMEOUT 30)
    endif()
    set_tests_properties(umicom.c.cli-budget PROPERTIES LABELS "umicom-c;cli" TIMEOUT 90)
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

# Unary ! composes existing runtime instructions and retains their wire profile.
add_executable(umicom-c-test-logical-not test_logical_not.c)
target_link_libraries(umicom-c-test-logical-not PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-logical-not PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
foreach(case IN ITEMS values trace operand-failure refusals)
    add_test(NAME "umicom.c.logical-not.${case}" COMMAND umicom-c-test-logical-not "${case}")
    set_tests_properties("umicom.c.logical-not.${case}" PROPERTIES LABELS "umicom-c;compiler;logical-not" TIMEOUT 15)
endforeach()

# The course fixture also passes through the installed-command entry points.
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-logical-not
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/logical_not.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/logical-not-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_logical_not.cmake")
    set_tests_properties(umicom.c.cli-logical-not PROPERTIES LABELS "umicom-c;cli;logical-not" TIMEOUT 60)
endif()


# Component queries must remain usable when the compiler is not linked.
foreach(component IN ITEMS compiler runtime)
    add_executable("umicom-c-test-${component}-profile" "test_${component}_profile.c")
    if(component STREQUAL "compiler")
        target_link_libraries("umicom-c-test-${component}-profile" PRIVATE UmicomC::Compiler)
    else()
        target_link_libraries("umicom-c-test-${component}-profile" PRIVATE UmicomC::Runtime)
    endif()
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options("umicom-c-test-${component}-profile" PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
    endif()
    add_test(NAME "umicom.c.profile.${component}" COMMAND "umicom-c-test-${component}-profile")
    set_tests_properties("umicom.c.profile.${component}" PROPERTIES TIMEOUT 15 LABELS "umicom-c;profile")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-profile
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_profile.cmake")
    set_tests_properties(umicom.c.cli-profile PROPERTIES TIMEOUT 30 LABELS "umicom-c;cli;profile")
endif()

# Numeric token spelling stays in the compiler; wire compatibility is checked
# with an independent fixture rather than merely decoding our own encoder.
add_executable(umicom-c-test-integer-literals test_integer_literals.c)
target_link_libraries(umicom-c-test-integer-literals PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-integer-literals PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS values refusals spans wire decimal)
    add_test(NAME "umicom.c.integer-literals.${case}"
        COMMAND umicom-c-test-integer-literals "${case}")
    set_tests_properties("umicom.c.integer-literals.${case}" PROPERTIES
        TIMEOUT 15 LABELS "umicom-c;compiler;literals")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-integer-literals
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/integer_literals.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/integer-literal-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_integer_literals.cmake")
    set_tests_properties(umicom.c.cli-integer-literals PROPERTIES
        TIMEOUT 60 LABELS "umicom-c;cli;literals")
endif()

# Configure-input identity is deliberately separate from binary provenance.
# Mutation fixtures use synthetic paths; they never rewrite the checkout or
# execute the synthetic tool whose bytes they compare.
foreach(case IN ITEMS unchanged literal-configuration-token source added-source deleted-source renamed-source
    header configuration example resource generated deleted-generated new-generated
    recipe tool missing-tool missing-receipt malformed-receipt trailing-receipt
    changed-receipt oversized-receipt receipt-as-code missing-recipe nonobject-recipe
    prose-outside-scope binary-outside-scope)
    add_test(NAME "umicom.c.input-identity.${case}"
        COMMAND "${CMAKE_COMMAND}"
            "-DMODULE=${PROJECT_SOURCE_DIR}/cmake/InputIdentity.cmake"
            "-DCHECKER=${PROJECT_SOURCE_DIR}/cmake/CheckInputIdentity.cmake"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/input-identity"
            "-DCASE=${case}"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_input_identity.cmake")
    set_tests_properties("umicom.c.input-identity.${case}" PROPERTIES
        TIMEOUT 60 LABELS "umicom-c;configuration-inputs;mutation")
endforeach()

# Keep operation identity distinct from program execution and result equality.
if(TARGET umicom-c)
    foreach(case IN ITEMS same comments radix different same-result unexecuted-failure
        invalid-left invalid-right missing-left missing-right oversized arity)
        add_test(NAME "umicom.c.cli-operation-identity.${case}"
            COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
                "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/operation-identity"
                "-DCASE=${case}" -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_operation_identity.cmake")
        set_tests_properties("umicom.c.cli-operation-identity.${case}" PROPERTIES
            TIMEOUT 30 LABELS "umicom-c;cli;comparison;read-only")
    endforeach()
endif()

# Report completion is part of the CLI result. Use the real entry and a native
# read-only descriptor so a short buffered printf must be explicitly flushed.
if(TARGET umicom-c AND (WIN32 OR UNIX))
    add_executable(umicom-c-test-report-failure test_cli_report_failure.c)
# This fixture includes the real CLI entry and must link every hosted command.
target_sources(umicom-c-test-report-failure PRIVATE ../tools/debugger.c)
    target_link_libraries(umicom-c-test-report-failure PRIVATE UmicomC::Compiler)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(umicom-c-test-report-failure PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
    endif()
    set(_report_dir "${CMAKE_CURRENT_BINARY_DIR}/report failure paths")
    file(MAKE_DIRECTORY "${_report_dir}")
    file(WRITE "${_report_dir}/left.uc" "int main(void){return 42;}\n")
    file(WRITE "${_report_dir}/right.uc" "int main(void){return 43;}\n")
    foreach(_report_semantic IN ITEMS same different)
        if(_report_semantic STREQUAL "same")
            set(_report_right "${_report_dir}/left.uc")
        else()
            set(_report_right "${_report_dir}/right.uc")
        endif()
        foreach(_report_mode IN ITEMS control deny)
            add_test(NAME "umicom.c.report.${_report_semantic}.${_report_mode}"
                COMMAND umicom-c-test-report-failure "${_report_dir}/left.uc"
                    "${_report_right}" "${_report_semantic}" "${_report_mode}")
            set_tests_properties("umicom.c.report.${_report_semantic}.${_report_mode}" PROPERTIES
                TIMEOUT 15 LABELS "umicom-c;cli;comparison;output-failure")
        endforeach()
    endforeach()
    foreach(_report_case IN ITEMS version profile check warnings run budget inspect)
        set(_report_command "${_report_case}")
        if(_report_case STREQUAL "version")
            set(_report_command "--version")
        endif()
        foreach(_report_mode IN ITEMS control deny)
            add_test(NAME "umicom.c.report.${_report_case}.${_report_mode}"
                COMMAND umicom-c-test-report-failure "${_report_dir}/left.uc"
                    "${_report_dir}/right.uc" "${_report_command}" "${_report_mode}")
            set_tests_properties("umicom.c.report.${_report_case}.${_report_mode}" PROPERTIES
                TIMEOUT 15 LABELS "umicom-c;cli;output-failure")
        endforeach()
    endforeach()
endif()

# Local storage has a separate source and runtime boundary. The latter target
# deliberately links no compiler, as the freestanding Kernel does.
foreach(component IN ITEMS source runtime)
    add_executable("umicom-c-test-local-${component}" "test_local_${component}.c")
    if(component STREQUAL "source")
        target_link_libraries("umicom-c-test-local-${component}" PRIVATE UmicomC::Compiler)
        set(local_cases values refusals bounds spans)
    else()
        target_link_libraries("umicom-c-test-local-${component}" PRIVATE UmicomC::Runtime)
        set(local_cases execution trace-fuel malformed wire)
    endif()
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options("umicom-c-test-local-${component}" PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
    endif()
    foreach(case IN LISTS local_cases)
        add_test(NAME "umicom.c.locals.${component}.${case}" COMMAND "umicom-c-test-local-${component}" "${case}")
        set_tests_properties("umicom.c.locals.${component}.${case}" PROPERTIES TIMEOUT 15 LABELS "umicom-c;locals")
    endforeach()
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-local-integers
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/local_integers.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/local-integer-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_local_integers.cmake")
    set_tests_properties(umicom.c.cli-local-integers PROPERTIES TIMEOUT 60 LABELS "umicom-c;cli;locals")
endif()

# Branch admission is tested without the compiler; source semantics also have
# an independent native-C oracle over a common, defined-behavior corpus.
foreach(group IN ITEMS short_circuit_source forward_branches logical_native)
    add_executable("umicom-c-test-${group}" "test_${group}.c")
    if(group STREQUAL "forward_branches")
        target_link_libraries("umicom-c-test-${group}" PRIVATE UmicomC::Runtime)
        set(logic_cases execution trace-fuel malformed wire)
    elseif(group STREQUAL "short_circuit_source")
        target_link_libraries("umicom-c-test-${group}" PRIVATE UmicomC::Compiler)
        set(logic_cases values faults refusals)
    else()
        target_link_libraries("umicom-c-test-${group}" PRIVATE UmicomC::Compiler)
        set(logic_cases native)
    endif()
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options("umicom-c-test-${group}" PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
    endif()
    foreach(case IN LISTS logic_cases)
        add_test(NAME "umicom.c.logical.${group}.${case}" COMMAND "umicom-c-test-${group}" "${case}")
        set_tests_properties("umicom.c.logical.${group}.${case}" PROPERTIES TIMEOUT 15 LABELS "umicom-c;logical;branches")
    endforeach()
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-short-circuit
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/short_circuit.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/short-circuit-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_short_circuit.cmake")
    set_tests_properties(umicom.c.cli-short-circuit PROPERTIES TIMEOUT 60 LABELS "umicom-c;cli;logical")
endif()

# Conditional grammar reuses the checked forward runtime and canonical format 4.
add_executable(umicom-c-test-conditional test_conditional.c)
target_link_libraries(umicom-c-test-conditional PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-conditional PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS values faults refusals bounds wire-trace)
    add_test(NAME "umicom.c.conditional.${case}" COMMAND umicom-c-test-conditional "${case}")
    set_tests_properties("umicom.c.conditional.${case}" PROPERTIES TIMEOUT 15 LABELS "umicom-c;conditional;compiler")
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-conditional
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/conditional_value.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/conditional-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_conditional.cmake")
    set_tests_properties(umicom.c.cli-conditional PROPERTIES TIMEOUT 60 LABELS "umicom-c;cli;conditional")
endif()

# Compound statements are a compiler extension using existing local/arithmetic
# records. Independent native arithmetic and literal wire fixtures guard it.
add_executable(umicom-c-test-compound-assignment test_compound_assignment.c)
target_link_libraries(umicom-c-test-compound-assignment PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-compound-assignment PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS values faults refusals spans bounds wire-trace native)
    add_test(NAME "umicom.c.compound-assignment.${case}"
        COMMAND umicom-c-test-compound-assignment "${case}")
    set_tests_properties("umicom.c.compound-assignment.${case}" PROPERTIES
        TIMEOUT 15 LABELS "umicom-c;compound-assignment;compiler")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-compound-assignment
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/compound_assignment.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/compound-assignment-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_compound_assignment.cmake")
    set_tests_properties(umicom.c.cli-compound-assignment PROPERTIES
        TIMEOUT 60 LABELS "umicom-c;compound-assignment;cli")
endif()

# Conditional statement syntax lowers to the existing forward graph. Literal
# wire and native-C fixtures check behavior independently of compiler output.
add_executable(umicom-c-test-if-statement test_if_statement.c)
target_link_libraries(umicom-c-test-if-statement PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-if-statement PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS values refusals faults spans nesting bounds wire-trace native)
    add_test(NAME "umicom.c.if-statement.${case}" COMMAND umicom-c-test-if-statement "${case}")
    set_tests_properties("umicom.c.if-statement.${case}" PROPERTIES
        TIMEOUT 15 LABELS "umicom-c;if-statement;compiler")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-if-statement
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/conditional_update.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/conditional-update-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_if_statement.cmake")
    set_tests_properties(umicom.c.cli-if-statement PROPERTIES
        TIMEOUT 60 LABELS "umicom-c;if-statement;cli")
endif()

# Chained conditions are source-only lowering with unchanged forward wire records.
add_executable(umicom-c-test-else-if test_else_if.c)
target_link_libraries(umicom-c-test-else-if PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-else-if PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS values refusals faults depth wire fuel native capacity locations)
    add_test(NAME "umicom.c.else-if.${case}" COMMAND umicom-c-test-else-if "${case}")
    set_tests_properties("umicom.c.else-if.${case}" PROPERTIES
        TIMEOUT 15 LABELS "umicom-c;else-if;compiler;branches")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-else-if
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/conditional_chain.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/conditional-chain-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_else_if.cmake")
    set_tests_properties(umicom.c.cli-else-if PROPERTIES TIMEOUT 60 LABELS "umicom-c;else-if;cli")
endif()

# Remainder shares one checked runtime operation across source and wire clients.
add_executable(umicom-c-test-remainder test_remainder.c)
target_link_libraries(umicom-c-test-remainder PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-remainder PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS values faults refusals wire versions fuel malformed spans native)
    add_test(NAME "umicom.c.remainder.${case}" COMMAND umicom-c-test-remainder "${case}")
    set_tests_properties("umicom.c.remainder.${case}" PROPERTIES
        TIMEOUT 20 LABELS "umicom-c;remainder;compiler;runtime")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-remainder
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/remainder.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/remainder-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_remainder.cmake")
    set_tests_properties(umicom.c.cli-remainder PROPERTIES TIMEOUT 60 LABELS "umicom-c;remainder;cli")
endif()

# Complement changes source lowering only. Wire and native oracles guard its
# endpoints, while bounded-depth tests exercise the unchanged runtime stack.
add_executable(umicom-c-test-complement test_complement.c)
target_link_libraries(umicom-c-test-complement PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-complement PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS values trace faults refusals spans capacity depth wire native)
    add_test(NAME "umicom.c.complement.${case}" COMMAND umicom-c-test-complement "${case}")
    set_tests_properties("umicom.c.complement.${case}" PROPERTIES
        TIMEOUT 20 LABELS "umicom-c;complement;compiler")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-complement
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/complement.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/complement-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_complement.cmake")
    set_tests_properties(umicom.c.cli-complement PROPERTIES TIMEOUT 60 LABELS "umicom-c;complement;cli")
endif()


# Eager bitwise values share one checked operation family across source and wire.
add_executable(umicom-c-test-bitwise test_bitwise.c)
target_link_libraries(umicom-c-test-bitwise PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-bitwise PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS values precedence updates refusals eager wire versions fuel malformed spans limits native)
    add_test(NAME "umicom.c.bitwise.${case}" COMMAND umicom-c-test-bitwise "${case}")
    set_tests_properties("umicom.c.bitwise.${case}" PROPERTIES
        TIMEOUT 20 LABELS "umicom-c;bitwise;compiler;runtime")
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-bitwise
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
            "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/bitwise.uc"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/bitwise-artifacts"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_bitwise.cmake")
    set_tests_properties(umicom.c.cli-bitwise PROPERTIES TIMEOUT 60 LABELS "umicom-c;bitwise;cli")
endif()


# Source-only ++/-- uses the established local slot and arithmetic runtime.
add_executable(umicom-c-test-increment test_increment.c)
target_link_libraries(umicom-c-test-increment PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-increment PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case values refusals faults wire-fuel spans bounds native)
    add_test(NAME "umicom.c.increment.${case}" COMMAND umicom-c-test-increment "${case}")
    set_tests_properties("umicom.c.increment.${case}" PROPERTIES
        TIMEOUT 20 LABELS "umicom-c;increment;compiler;runtime")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-increment COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/increment.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/increment-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_increment.cmake")
    set_tests_properties(umicom.c.cli-increment PROPERTIES TIMEOUT 60 LABELS "umicom-c;increment;cli")
endif()

# Resumable execution uses the same engine as whole runs and traces.
add_executable(umicom-c-test-machine test_machine.c)
target_link_libraries(umicom-c-test-machine PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-machine PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case sequence faults fuel refusal parity locals)
    add_test(NAME "umicom.c.machine.${case}" COMMAND umicom-c-test-machine "${case}")
    set_tests_properties("umicom.c.machine.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;machine;runtime")
endforeach()

if(TARGET umicom-c)
    foreach(case steps fault fuel cancel eof unknown long syntax bytecode)
        add_test(NAME "umicom.c.debugger.${case}" COMMAND "${CMAKE_COMMAND}"
            "-DTOOL=$<TARGET_FILE:umicom-c>" "-DCASE=${case}"
            "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/debugger-${case}"
            -P "${CMAKE_CURRENT_LIST_DIR}/check_debugger.cmake")
        set_tests_properties("umicom.c.debugger.${case}" PROPERTIES TIMEOUT 30 LABELS "umicom-c;debugger;cli")
    endforeach()
endif()

add_executable(umicom-c-test-managed-heap test_managed_heap.c)
target_link_libraries(umicom-c-test-managed-heap PRIVATE UmicomC::ManagedHeap)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-managed-heap PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case cycle bounds atomic exhausted payload graphs)
    add_test(NAME "umicom.c.managed-heap.${case}" COMMAND umicom-c-test-managed-heap "${case}")
    set_tests_properties("umicom.c.managed-heap.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;managed-heap")
endforeach()

add_executable(umicom-c-test-source-regions test_source_regions.c)
target_link_libraries(umicom-c-test-source-regions PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-source-regions PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case regions protected malformed bounds)
    add_test(NAME "umicom.c.source-regions.${case}" COMMAND umicom-c-test-source-regions "${case}")
    set_tests_properties("umicom.c.source-regions.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;language-regions")
endforeach()

# Compile the public example as a regression so its documented ownership remains real.
add_executable(umicom-c-managed-cycle ../examples/managed_cycle.c)
target_link_libraries(umicom-c-managed-cycle PRIVATE UmicomC::ManagedHeap)
add_test(NAME umicom.c.managed-heap.example COMMAND umicom-c-managed-cycle)
set_tests_properties(umicom.c.managed-heap.example PROPERTIES TIMEOUT 15
    PASS_REGULAR_EXPRESSION "Unrooted cycle collected: 2; old handle refused")

if(TARGET umicom-c)
    add_test(NAME umicom.c.source-regions.cli COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/language_regions.umc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/language-regions-cli"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_source_regions.cmake")
    set_tests_properties(umicom.c.source-regions.cli PROPERTIES TIMEOUT 20 LABELS "umicom-c;language-regions;cli")
endif()

# Slices bound scheduling work while retaining lifetime VM fuel.
add_executable(umicom-c-test-machine-slices test_machine_slices.c)
target_link_libraries(umicom-c-test-machine-slices PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-machine-slices PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case sizes faults fuel refusal parity)
    add_test(NAME "umicom.c.machine-slices.${case}" COMMAND umicom-c-test-machine-slices "${case}")
    set_tests_properties("umicom.c.machine-slices.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;machine;slices")
endforeach()

# Digit separators are lexical spelling; wire and runtime operations are retained.
add_executable(umicom-c-test-digit-separators test_digit_separators.c)
target_link_libraries(umicom-c-test-digit-separators PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-digit-separators PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case values refusals overflow spans wire)
    add_test(NAME "umicom.c.digit-separators.${case}" COMMAND umicom-c-test-digit-separators "${case}")
    set_tests_properties("umicom.c.digit-separators.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;literals")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-digit-separators COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/digit_separators.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/digit-separator-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_integer_literals.cmake")
    set_tests_properties(umicom.c.cli-digit-separators PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;literals;cli")
endif()

# BASIC comments affect lexical region admission only; no foreign code executes.
add_executable(umicom-c-test-basic-comment-regions test_basic_comment_regions.c)
target_link_libraries(umicom-c-test-basic-comment-regions PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-basic-comment-regions PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case line-comments colon-comments keyword-boundaries quotes-and-comments other-languages bounded-span)
    add_test(NAME "umicom.c.source-regions.basic.${case}" COMMAND umicom-c-test-basic-comment-regions "${case}")
    set_tests_properties("umicom.c.source-regions.basic.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;language-regions;basic")
endforeach()

# Read-only locals are compiler admission, not a bytecode mutation permission.
add_executable(umicom-c-test-readonly-locals test_readonly_locals.c)
target_link_libraries(umicom-c-test-readonly-locals PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-readonly-locals PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case values writes syntax faults wire spans limits native)
    add_test(NAME "umicom.c.readonly-locals.${case}" COMMAND umicom-c-test-readonly-locals "${case}")
    set_tests_properties("umicom.c.readonly-locals.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;locals")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-readonly-locals COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/readonly_locals.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/readonly-local-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_readonly_locals.cmake")
    set_tests_properties(umicom.c.cli-readonly-locals PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;locals;cli")
endif()

# Lexical branch names lower through existing slot and forward-branch owners.
add_executable(umicom-c-test-branch-locals test_branch_locals.c)
target_link_libraries(umicom-c-test-branch-locals PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-branch-locals PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case values visibility wire limits reuse native faults spans depth)
    add_test(NAME "umicom.c.branch-locals.${case}" COMMAND umicom-c-test-branch-locals "${case}")
    set_tests_properties("umicom.c.branch-locals.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;locals;scope")
endforeach()

# The public CLI must preserve the same lexical visibility and wire contract.
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-branch-locals COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/branch_locals.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/branch-local-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_branch_locals.cmake")
    set_tests_properties(umicom.c.cli-branch-locals PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;scope;cli")
endif()

# Standalone scopes share branch-body ownership without runtime additions.
add_executable(umicom-c-test-compound-statements test_compound_statements.c)
target_link_libraries(umicom-c-test-compound-statements PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-compound-statements PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case values visibility refusals limits reuse depth wire faults spans native)
    add_test(NAME "umicom.c.compound-statements.${case}" COMMAND umicom-c-test-compound-statements "${case}")
    set_tests_properties("umicom.c.compound-statements.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;locals;scope")
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-compound-statements COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/compound_statements.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/compound-statement-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_compound_statements.cmake")
    set_tests_properties(umicom.c.cli-compound-statements PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;scope;cli")
endif()

# Source warnings share the compiler; these cases never require a GUI or installed SDK.
add_executable(umicom-c-test-source-warnings test_source_warnings.c)
target_link_libraries(umicom-c-test-source-warnings PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-source-warnings PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case reads updates branches scopes positions refusals limits spans no-execution repeat)
    add_test(NAME "umicom.c.source-warnings.${case}" COMMAND umicom-c-test-source-warnings "${case}")
    set_tests_properties("umicom.c.source-warnings.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;warnings")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-source-warnings COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/source-warning-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_source_warnings.cmake")
    set_tests_properties(umicom.c.cli-source-warnings PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;warnings;cli")
endif()

# Package discovery is checked separately from installed archive/link qualification.
add_subdirectory(package_components)

# Checked shifts share the canonical compiler/runtime and exercise wire format 7.
add_executable(umicom-c-test-shifts test_shifts.c)
target_link_libraries(umicom-c-test-shifts PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-shifts PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
foreach(case IN ITEMS values precedence updates refusals faults wire versions malformed fuel spans native)
    add_test(NAME "umicom.c.shifts.${case}" COMMAND umicom-c-test-shifts "${case}")
    set_tests_properties("umicom.c.shifts.${case}" PROPERTIES LABELS "umicom-c;compiler;runtime;shifts" TIMEOUT 15)
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-shifts
        COMMAND "${CMAKE_COMMAND}" "-DTOOL=$<TARGET_FILE:umicom-c>"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/shift-artifacts"
        "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/checked_shifts.uc"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_shifts.cmake")
    set_tests_properties(umicom.c.cli-shifts PROPERTIES LABELS "umicom-c;cli;shifts" TIMEOUT 45)
endif()

# Check graph inspection independently from collector mutation.
add_executable(umicom-c-test-heap-inspection test_heap_inspection.c)
target_link_libraries(umicom-c-test-heap-inspection PRIVATE UmicomC::ManagedHeap)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-heap-inspection PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case totals cycles refusal retirement graphs)
    add_test(NAME "umicom.c.heap-inspection.${case}" COMMAND umicom-c-test-heap-inspection "${case}")
    set_tests_properties("umicom.c.heap-inspection.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;managed-heap;inspection")
endforeach()

# Empty source statements consume no runtime records or fuel.
add_executable(umicom-c-test-empty-statements test_empty_statements.c)
target_link_libraries(umicom-c-test-empty-statements PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-empty-statements PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case values cost wire refusals many warnings spans)
    add_test(NAME "umicom.c.empty-statements.${case}" COMMAND umicom-c-test-empty-statements "${case}")
    set_tests_properties("umicom.c.empty-statements.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;grammar;ownership")
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-empty-statements COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/empty_statements.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/empty-statement-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_empty_statements.cmake")
    set_tests_properties(umicom.c.cli-empty-statements PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;grammar;cli")
endif()

# ASCII character tokens lower through the existing integer runtime.
add_executable(umicom-c-test-ascii-characters test_ascii_characters.c)
target_link_libraries(umicom-c-test-ascii-characters PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-ascii-characters PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case printable escapes numeric refusals ranges spans wire warnings long-escape)
    add_test(NAME "umicom.c.ascii-characters.${case}" COMMAND umicom-c-test-ascii-characters "${case}")
    set_tests_properties("umicom.c.ascii-characters.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;grammar;characters")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-ascii-characters COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/ascii_characters.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/ascii-character-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_ascii_characters.cmake")
    set_tests_properties(umicom.c.cli-ascii-characters PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;grammar;cli")
endif()

# Declaration lists share one local owner and require no new runtime opcode.
add_executable(umicom-c-test-declarator-lists test_declarator_lists.c)
target_link_libraries(umicom-c-test-declarator-lists PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-declarator-lists PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case values scope qualifiers refusals parity capacity warnings spans wire faults)
    add_test(NAME "umicom.c.declarator-lists.${case}" COMMAND umicom-c-test-declarator-lists "${case}")
    set_tests_properties("umicom.c.declarator-lists.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;grammar;locals")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-declarator-lists COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/declarator_lists.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/declarator-list-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_declarator_lists.cmake")
    set_tests_properties(umicom.c.cli-declarator-lists PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;grammar;cli")
endif()

# Structural path budgets do not execute source or alter existing inspection output.
add_executable(umicom-c-test-execution-bounds test_execution_bounds.c)
target_link_libraries(umicom-c-test-execution-bounds PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-execution-bounds PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS straight branches fuel source-paths conservative arithmetic refusal malformed maximum wire)
    add_test(NAME "umicom.c.execution-bounds.${case}" COMMAND umicom-c-test-execution-bounds "${case}")
    set_tests_properties("umicom.c.execution-bounds.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;runtime;inspection;fuel")
endforeach()

if(UMICOM_C_BUILD_TOOLS)
    add_test(NAME umicom.c.cli-execution-bounds COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/path_budget.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/execution-bounds-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_execution_bounds.cmake")
    set_tests_properties(umicom.c.cli-execution-bounds PROPERTIES TIMEOUT 60 LABELS "umicom-c;cli;inspection;fuel")
endif()

# Source excerpts are compiler presentation; no VM or wire profile is changed.
add_executable(umicom-c-test-diagnostic-excerpt test_diagnostic_excerpt.c)
target_link_libraries(umicom-c-test-diagnostic-excerpt PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-diagnostic-excerpt PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case IN ITEMS basic empty newlines escapes all-bytes window maximum refusal surrounding-lines)
    add_test(NAME "umicom.c.diagnostic-excerpt.${case}" COMMAND umicom-c-test-diagnostic-excerpt "${case}")
    set_tests_properties("umicom.c.diagnostic-excerpt.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;diagnostics")
endforeach()
if(UMICOM_C_BUILD_TOOLS)
    add_test(NAME umicom.c.cli-diagnostic-excerpt COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/excerpt-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_diagnostic_excerpt.cmake")
    set_tests_properties(umicom.c.cli-diagnostic-excerpt PROPERTIES TIMEOUT 60 LABELS "umicom-c;cli;diagnostics")
endif()

# Nested BASIC comments remain lexical classification, with bounded iterative state.
add_executable(umicom-c-test-basic-block-regions test_basic_block_regions.c)
target_link_libraries(umicom-c-test-basic-block-regions PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-basic-block-regions PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case ordinary nested protected-markers statement-state depth-boundary depth-overflow unterminated bounded-span language-isolation source-and-output-bounds)
    add_test(NAME "umicom.c.source-regions.basic-block.${case}" COMMAND umicom-c-test-basic-block-regions "${case}")
    set_tests_properties("umicom.c.source-regions.basic-block.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;language-regions;basic")
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.source-regions.basic-block.cli COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/basic_block_comments.umc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/basic-block-cli"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_basic_block_regions.cmake")
    set_tests_properties(umicom.c.source-regions.basic-block.cli PROPERTIES TIMEOUT 30 LABELS "umicom-c;language-regions;basic;cli")
endif()

# Boolean keyword spelling uses the existing integer operations and wire format.
add_executable(umicom-c-test-boolean-literals test_boolean_literals.c)
target_link_libraries(umicom-c-test-boolean-literals PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-boolean-literals PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case values native-parity keywords scope warnings spans wire fuel faults)
    add_test(NAME "umicom.c.boolean-literals.${case}" COMMAND umicom-c-test-boolean-literals "${case}")
    set_tests_properties("umicom.c.boolean-literals.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;compiler;grammar;boolean")
endforeach()
if(TARGET umicom-c)
    add_test(NAME umicom.c.cli-boolean-literals COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/boolean_literals.uc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/boolean-literal-artifacts"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_cli_boolean_literals.cmake")
    set_tests_properties(umicom.c.cli-boolean-literals PROPERTIES TIMEOUT 60 LABELS "umicom-c;compiler;grammar;cli")
endif()

# D envelopes classify boundaries; no foreign frontend or execution is added.
add_executable(umicom-c-test-d-regions test_d_regions.c)
target_link_libraries(umicom-c-test-d-regions PRIVATE UmicomC::Compiler)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(umicom-c-test-d-regions PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
foreach(case identity ordinary strings comments prefix-boundary unsupported-strings depth-boundary
    depth-overflow unterminated bounded-span language-isolation source-bounds tags classification-only)
    add_test(NAME "umicom.c.source-regions.d.${case}" COMMAND umicom-c-test-d-regions "${case}")
    set_tests_properties("umicom.c.source-regions.d.${case}" PROPERTIES TIMEOUT 20 LABELS "umicom-c;language-regions;d")
endforeach()

if(TARGET umicom-c)
    add_test(NAME umicom.c.source-regions.d.cli COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:umicom-c>" "-DSOURCE=${PROJECT_SOURCE_DIR}/examples/d_regions.umc"
        "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/d-regions-cli"
        -P "${CMAKE_CURRENT_LIST_DIR}/check_d_regions.cmake")
    set_tests_properties(umicom.c.source-regions.d.cli PROPERTIES TIMEOUT 30 LABELS "umicom-c;language-regions;d;cli")
endif()
