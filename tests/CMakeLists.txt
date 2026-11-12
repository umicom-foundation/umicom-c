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
