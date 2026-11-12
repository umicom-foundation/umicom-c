/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/status.c
 *
 * PURPOSE:
 *   Provide allocation-free diagnostics shared by hosted and freestanding execution.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "internal.h"
#include "umicom_c/profile.h"

const char *umc_status_text(UmcStatus status)
{
    switch (status) {
    case UMC_OK: return "success";
    case UMC_INVALID_ARGUMENT: return "invalid argument";
    case UMC_SOURCE_TOO_LARGE: return "source exceeds the supported byte limit";
    case UMC_SYNTAX_ERROR: return "unsupported or malformed source";
    case UMC_LITERAL_RANGE: return "decimal literal exceeds INT32_MAX";
    case UMC_PROGRAM_TOO_LARGE: return "instruction capacity exceeded";
    case UMC_DEPTH_LIMIT: return "expression nesting limit exceeded";
    case UMC_BAD_BYTECODE: return "invalid bytecode structure";
    case UMC_STACK_OVERFLOW: return "operand stack capacity exceeded";
    case UMC_FUEL_EXHAUSTED: return "instruction budget exhausted";
    case UMC_ARITHMETIC_ERROR: return "division by zero or signed arithmetic overflow";
    case UMC_BUFFER_TOO_SMALL: return "output buffer is too small";
    case UMC_LOCAL_CAPACITY: return "local variable capacity exceeded";
    default: return "unknown status";
    }
}

const char *umc_opcode_text(UmcOpcode opcode)
{
    switch (opcode) {
    case UMC_PUSH: return "push";
    case UMC_ADD: return "add";
    case UMC_SUBTRACT: return "subtract";
    case UMC_MULTIPLY: return "multiply";
    case UMC_DIVIDE: return "divide";
    case UMC_NEGATE: return "negate";
    case UMC_RETURN: return "return";
    case UMC_LESS: return "less";
    case UMC_LESS_EQUAL: return "less-equal";
    case UMC_GREATER: return "greater";
    case UMC_GREATER_EQUAL: return "greater-equal";
    case UMC_EQUAL: return "equal";
    case UMC_NOT_EQUAL: return "not-equal";
    case UMC_LOAD_LOCAL: return "load-local";
    case UMC_STORE_LOCAL: return "store-local";
    case UMC_JUMP: return "jump";
    case UMC_JUMP_IF_ZERO: return "jump-if-zero";
    default: return "invalid";
    }
}

UmcStatus umc_fail(UmcDiagnostic *diagnostic, UmcStatus status, size_t offset)
{
    if (diagnostic != NULL) {
        diagnostic->status = status;
        diagnostic->source_offset = offset;
        diagnostic->line = 0U;
        diagnostic->column = 0U;
        diagnostic->message = umc_status_text(status);
    }
    return status;
}

void umc_locate_diagnostic(const char *source, size_t length, UmcDiagnostic *diagnostic)
{
    if (diagnostic == NULL || source == NULL || length > UMC_SOURCE_LIMIT) {
        return;
    }
    size_t end = diagnostic->source_offset;
    if (end > length) {
        end = length;
    }
    size_t line = 1U;
    size_t column = 1U;
    for (size_t i = 0U; i < end; ++i) {
        if (source[i] == '\r') {
            ++line;
            column = 1U;
            if (i + 1U < end && source[i + 1U] == '\n') {
                ++i;
            }
        } else if (source[i] == '\n') {
            ++line;
            column = 1U;
        } else {
            ++column;
        }
    }
    diagnostic->line = line;
    diagnostic->column = column;
}


/* Keep runtime discovery in an object already shared by hosted and freestanding
 * consumers. Return values come from this library's compilation, rather than
 * from constants expanded in a separately built caller. */
/* Report local slots and the canonical local bytecode profile from the linked runtime. The predecessor remains disabled for review. */
#if 0
uint32_t umc_runtime_profile(UmcProfileProperty property)
{
    switch (property) {
    case UMC_PROFILE_API: return UMC_API_VERSION;
    case UMC_PROFILE_FEATURES:
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON |
            UMC_FEATURE_EXECUTION_TRACE | UMC_FEATURE_PROGRAM_INSPECTION;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_STACK_VALUES: return UMC_STACK_LIMIT;
    case UMC_PROFILE_BYTECODE_BYTES: return UMC_BYTECODE_LIMIT;
    case UMC_PROFILE_WIRE_VERSIONS: return (UINT32_C(1) << 1U) | (UINT32_C(1) << 2U);
    default: return 0U;
    }
}

#endif

/* Report runtime graph support independently from source spelling features. The predecessor remains disabled for review. */
#if 0
uint32_t umc_runtime_profile(UmcProfileProperty property)
{
    switch (property) {
    case UMC_PROFILE_API: return UMC_API_VERSION;
    case UMC_PROFILE_FEATURES:
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON |
            UMC_FEATURE_EXECUTION_TRACE | UMC_FEATURE_PROGRAM_INSPECTION | UMC_FEATURE_LOCAL_INTEGERS;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_STACK_VALUES: return UMC_STACK_LIMIT;
    case UMC_PROFILE_BYTECODE_BYTES: return UMC_BYTECODE_LIMIT;
    case UMC_PROFILE_WIRE_VERSIONS: return (UINT32_C(1) << 1U) | (UINT32_C(1) << 2U) | (UINT32_C(1) << 3U);
    case UMC_PROFILE_LOCAL_VALUES: return UMC_LOCAL_LIMIT;
    default: return 0U;
    }
}

#endif

uint32_t umc_runtime_profile(UmcProfileProperty property)
{
    switch (property) {
    case UMC_PROFILE_API: return UMC_API_VERSION;
    case UMC_PROFILE_FEATURES:
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON |
            UMC_FEATURE_EXECUTION_TRACE | UMC_FEATURE_PROGRAM_INSPECTION | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_FORWARD_BRANCHES;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_STACK_VALUES: return UMC_STACK_LIMIT;
    case UMC_PROFILE_BYTECODE_BYTES: return UMC_BYTECODE_LIMIT;
    case UMC_PROFILE_WIRE_VERSIONS: return (UINT32_C(1) << 1U) | (UINT32_C(1) << 2U) | (UINT32_C(1) << 3U) | (UINT32_C(1) << 4U);
    case UMC_PROFILE_LOCAL_VALUES: return UMC_LOCAL_LIMIT;
    default: return 0U;
    }
}
