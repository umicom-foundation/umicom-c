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
