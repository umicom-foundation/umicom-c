/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/vm.c
 *
 * PURPOSE:
 *   Validate every instruction before bounded signed-arithmetic execution.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "internal.h"
#include <limits.h>

UmcStatus umc_validate(const UmcInstruction *program, size_t count, UmcDiagnostic *diagnostic)
{
    (void)umc_fail(diagnostic, UMC_OK, 0U);
    if (program == NULL || count == 0U || count > UMC_PROGRAM_LIMIT) {
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    }
    size_t depth = 0U;
    for (size_t i = 0U; i < count; ++i) {
        const UmcInstruction *instruction = &program[i];
        const size_t offset = instruction->source_offset;
        if (offset > UMC_SOURCE_LIMIT ||
            (instruction->opcode != UMC_PUSH && instruction->operand != 0)) {
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        }
        switch (instruction->opcode) {
        case UMC_PUSH:
            if (depth == UMC_STACK_LIMIT) {
                return umc_fail(diagnostic, UMC_STACK_OVERFLOW, offset);
            }
            ++depth;
            break;
        case UMC_ADD:
        case UMC_SUBTRACT:
        case UMC_MULTIPLY:
        case UMC_DIVIDE:
            if (depth < 2U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            break;
        case UMC_NEGATE:
            if (depth == 0U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            break;
        case UMC_RETURN:
            if (depth != 1U || i + 1U != count) {
                return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            }
            break;
        default:
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        }
    }
    if (program[count - 1U].opcode != UMC_RETURN) {
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, program[count - 1U].source_offset);
    }
    return UMC_OK;
}

UmcStatus umc_execute(const UmcInstruction *program, size_t count, size_t fuel,
    UmcResult *result, UmcDiagnostic *diagnostic)
{
    if (result == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    const UmcStatus shape = umc_validate(program, count, diagnostic);
    if (shape != UMC_OK) return shape;
    /* No aggregate initialization, allocation, libc calls or host capabilities:
     * the same runtime can be linked as a freestanding native program. Every
     * stack slot is written before its validated instruction sequence reads it. */
    int32_t stack[UMC_STACK_LIMIT];
    size_t depth = 0U;
    for (size_t pc = 0U; pc < count; ++pc) {
        const UmcInstruction *instruction = &program[pc];
        if (pc >= fuel) {
            return umc_fail(diagnostic, UMC_FUEL_EXHAUSTED, instruction->source_offset);
        }
        if (instruction->opcode == UMC_PUSH) {
            stack[depth++] = instruction->operand;
        } else if (instruction->opcode == UMC_RETURN) {
            result->value = stack[0];
            result->instructions_executed = pc + 1U;
            return UMC_OK;
        } else if (instruction->opcode == UMC_NEGATE) {
            if (stack[depth - 1U] == INT32_MIN) {
                return umc_fail(diagnostic, UMC_ARITHMETIC_ERROR, instruction->source_offset);
            }
            stack[depth - 1U] = -stack[depth - 1U];
        } else {
            const int32_t right = stack[--depth];
            const int32_t left = stack[depth - 1U];
            int64_t value;
            switch (instruction->opcode) {
            case UMC_ADD: value = (int64_t)left + (int64_t)right; break;
            case UMC_SUBTRACT: value = (int64_t)left - (int64_t)right; break;
            case UMC_MULTIPLY: value = (int64_t)left * (int64_t)right; break;
            case UMC_DIVIDE:
                if (right == 0 || (left == INT32_MIN && right == -1)) {
                    return umc_fail(diagnostic, UMC_ARITHMETIC_ERROR, instruction->source_offset);
                }
                value = left / right;
                break;
            default: return umc_fail(diagnostic, UMC_BAD_BYTECODE, instruction->source_offset);
            }
            /* Widen first, test the range, then narrow: even INT32_MIN squared
             * fits int64_t. Never rely on undefined signed overflow as a check. */
            if (value < INT32_MIN || value > INT32_MAX) {
                return umc_fail(diagnostic, UMC_ARITHMETIC_ERROR, instruction->source_offset);
            }
            stack[depth - 1U] = (int32_t)value;
        }
    }
    return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
}
