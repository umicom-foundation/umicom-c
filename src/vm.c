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
#include "umicom_c/trace.h"
#include <limits.h>

/* The original validator is retained for engineering review. The shared walk
 * below preserves its rejection rules while collecting resource facts for
 * CLI and IDE consumers, keeping execution and inspection in agreement. */
#if 0
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

#endif

/* Use the same complete stack admission for arithmetic and comparison instructions. The former implementation is retained for review. */
#if 0
static UmcStatus inspect_program(const UmcInstruction *program, size_t count,
    UmcProgramInfo *info, UmcDiagnostic *diagnostic)
{
    (void)umc_fail(diagnostic, UMC_OK, 0U);
    if (program == NULL || count == 0U || count > UMC_PROGRAM_LIMIT) {
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    }
    size_t depth = 0U;
    size_t peak = 0U;
    size_t constants = 0U;
    size_t arithmetic = 0U;
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
            ++constants;
            if (depth > peak) peak = depth;
            break;
        case UMC_ADD:
        case UMC_SUBTRACT:
        case UMC_MULTIPLY:
        case UMC_DIVIDE:
            if (depth < 2U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            ++arithmetic;
            break;
        case UMC_NEGATE:
            if (depth == 0U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            ++arithmetic;
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
    /* Publish only after the final instruction and every operand are checked.
     * Counts are bounded by UMC_PROGRAM_LIMIT, so the wire-size calculation
     * cannot overflow size_t on any supported runtime target. */
    if (info != NULL) {
        info->instruction_count = count;
        info->peak_stack = peak;
        info->constant_count = constants;
        info->arithmetic_count = arithmetic;
        info->bytecode_size = UMC_BYTECODE_HEADER_SIZE + count * UMC_BYTECODE_RECORD_SIZE;
    }
    return UMC_OK;
}
#endif

static UmcStatus inspect_program(const UmcInstruction *program, size_t count,
    UmcProgramInfo *info, UmcDiagnostic *diagnostic)
{
    (void)umc_fail(diagnostic, UMC_OK, 0U);
    if (program == NULL || count == 0U || count > UMC_PROGRAM_LIMIT) {
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    }
    size_t depth = 0U;
    size_t peak = 0U;
    size_t constants = 0U;
    size_t arithmetic = 0U;
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
            ++constants;
            if (depth > peak) peak = depth;
            break;
        case UMC_ADD:
        case UMC_SUBTRACT:
        case UMC_MULTIPLY:
        case UMC_DIVIDE:
            if (depth < 2U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            ++arithmetic;
            break;
        case UMC_LESS: case UMC_LESS_EQUAL: case UMC_GREATER:
        case UMC_GREATER_EQUAL: case UMC_EQUAL: case UMC_NOT_EQUAL:
            if (depth < 2U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            /* Keep arithmetic_count literal: comparisons are separate kinds
             * of operations, already included in instruction_count. */
            break;
        case UMC_NEGATE:
            if (depth == 0U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            ++arithmetic;
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
    /* Publish only after the final instruction and every operand are checked.
     * Counts are bounded by UMC_PROGRAM_LIMIT, so the wire-size calculation
     * cannot overflow size_t on any supported runtime target. */
    if (info != NULL) {
        info->instruction_count = count;
        info->peak_stack = peak;
        info->constant_count = constants;
        info->arithmetic_count = arithmetic;
        info->bytecode_size = UMC_BYTECODE_HEADER_SIZE + count * UMC_BYTECODE_RECORD_SIZE;
    }
    return UMC_OK;
}

UmcStatus umc_validate(const UmcInstruction *program, size_t count, UmcDiagnostic *diagnostic)
{
    return inspect_program(program, count, NULL, diagnostic);
}

UmcStatus umc_inspect(const UmcInstruction *program, size_t count,
    UmcProgramInfo *info, UmcDiagnostic *diagnostic)
{
    if (info == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    return inspect_program(program, count, info, diagnostic);
}

/* One arithmetic executor serves ordinary and traced clients. The previous
 * executor remains below for engineering comparison; it is replaced so CLI,
 * Framework and freestanding consumers cannot drift in overflow or fuel rules. */
#if 0
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
#endif

static void record_step(UmcTraceStep *steps, size_t *trace_count, size_t pc,
    const UmcInstruction *instruction, const int32_t *stack, size_t depth,
    UmcStatus status)
{
    if (steps == NULL) return;
    UmcTraceStep *step = &steps[*trace_count];
    step->instruction_index = pc;
    step->source_offset = instruction->source_offset;
    step->opcode = instruction->opcode;
    step->stack_depth = depth;
    step->top = depth == 0U ? 0 : stack[depth - 1U];
    step->status = status;
    ++*trace_count;
}

/* Shape validation and, for tracing, storage preflight precede this walk. Each
 * validated instruction produces at most one entry. Arithmetic checks precede
 * mutation, giving a failed trace entry an intact view of the attempted stack. */
/* Execute comparisons in the existing bounded engine so traced and ordinary callers share exactly one result policy. The former implementation is retained for review. */
#if 0
static UmcStatus execute_validated(const UmcInstruction *program, size_t count,
    size_t fuel, UmcResult *result, UmcTraceStep *steps, size_t *trace_count,
    UmcDiagnostic *diagnostic)
{
    int32_t stack[UMC_STACK_LIMIT];
    size_t depth = 0U;
    for (size_t pc = 0U; pc < count; ++pc) {
        const UmcInstruction *instruction = &program[pc];
        UmcStatus status = UMC_OK;
        if (pc >= fuel) {
            status = UMC_FUEL_EXHAUSTED;
        } else if (instruction->opcode == UMC_PUSH) {
            stack[depth++] = instruction->operand;
        } else if (instruction->opcode == UMC_RETURN) {
            record_step(steps, trace_count, pc, instruction, stack, depth, UMC_OK);
            result->value = stack[0];
            result->instructions_executed = pc + 1U;
            return UMC_OK;
        } else if (instruction->opcode == UMC_NEGATE) {
            if (stack[depth - 1U] == INT32_MIN) status = UMC_ARITHMETIC_ERROR;
            else stack[depth - 1U] = -stack[depth - 1U];
        } else {
            const int32_t right = stack[depth - 1U];
            const int32_t left = stack[depth - 2U];
            int64_t value = 0;
            switch (instruction->opcode) {
            case UMC_ADD: value = (int64_t)left + (int64_t)right; break;
            case UMC_SUBTRACT: value = (int64_t)left - (int64_t)right; break;
            case UMC_MULTIPLY: value = (int64_t)left * (int64_t)right; break;
            case UMC_DIVIDE:
                if (right == 0 || (left == INT32_MIN && right == -1))
                    status = UMC_ARITHMETIC_ERROR;
                else value = left / right;
                break;
            default: status = UMC_BAD_BYTECODE; break;
            }
            /* Widen first, check the range, then publish both value and depth.
             * Even INT32_MIN squared fits int64_t; signed overflow is never used
             * as an error detector. Every stack read follows shape validation. */
            if (status == UMC_OK && (value < INT32_MIN || value > INT32_MAX))
                status = UMC_ARITHMETIC_ERROR;
            if (status == UMC_OK) {
                --depth;
                stack[depth - 1U] = (int32_t)value;
            }
        }
        record_step(steps, trace_count, pc, instruction, stack, depth, status);
        if (status != UMC_OK)
            return umc_fail(diagnostic, status, instruction->source_offset);
    }
    return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
}
#endif

static UmcStatus execute_validated(const UmcInstruction *program, size_t count,
    size_t fuel, UmcResult *result, UmcTraceStep *steps, size_t *trace_count,
    UmcDiagnostic *diagnostic)
{
    int32_t stack[UMC_STACK_LIMIT];
    size_t depth = 0U;
    for (size_t pc = 0U; pc < count; ++pc) {
        const UmcInstruction *instruction = &program[pc];
        UmcStatus status = UMC_OK;
        if (pc >= fuel) {
            status = UMC_FUEL_EXHAUSTED;
        } else if (instruction->opcode == UMC_PUSH) {
            stack[depth++] = instruction->operand;
        } else if (instruction->opcode == UMC_RETURN) {
            record_step(steps, trace_count, pc, instruction, stack, depth, UMC_OK);
            result->value = stack[0];
            result->instructions_executed = pc + 1U;
            return UMC_OK;
        } else if (instruction->opcode == UMC_NEGATE) {
            if (stack[depth - 1U] == INT32_MIN) status = UMC_ARITHMETIC_ERROR;
            else stack[depth - 1U] = -stack[depth - 1U];
        } else {
            const int32_t right = stack[depth - 1U];
            const int32_t left = stack[depth - 2U];
            int64_t value = 0;
            switch (instruction->opcode) {
            case UMC_ADD: value = (int64_t)left + (int64_t)right; break;
            case UMC_SUBTRACT: value = (int64_t)left - (int64_t)right; break;
            case UMC_MULTIPLY: value = (int64_t)left * (int64_t)right; break;
            case UMC_DIVIDE:
                if (right == 0 || (left == INT32_MIN && right == -1))
                    status = UMC_ARITHMETIC_ERROR;
                else value = left / right;
                break;
            /* Native signed comparisons are defined across the whole int32
             * range. No subtraction is used to compare extreme values. */
            case UMC_LESS: value = left < right ? 1 : 0; break;
            case UMC_LESS_EQUAL: value = left <= right ? 1 : 0; break;
            case UMC_GREATER: value = left > right ? 1 : 0; break;
            case UMC_GREATER_EQUAL: value = left >= right ? 1 : 0; break;
            case UMC_EQUAL: value = left == right ? 1 : 0; break;
            case UMC_NOT_EQUAL: value = left != right ? 1 : 0; break;
            default: status = UMC_BAD_BYTECODE; break;
            }
            /* Widen first, check the range, then publish both value and depth.
             * Even INT32_MIN squared fits int64_t; signed overflow is never used
             * as an error detector. Every stack read follows shape validation. */
            if (status == UMC_OK && (value < INT32_MIN || value > INT32_MAX))
                status = UMC_ARITHMETIC_ERROR;
            if (status == UMC_OK) {
                --depth;
                stack[depth - 1U] = (int32_t)value;
            }
        }
        record_step(steps, trace_count, pc, instruction, stack, depth, status);
        if (status != UMC_OK)
            return umc_fail(diagnostic, status, instruction->source_offset);
    }
    return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
}

UmcStatus umc_execute(const UmcInstruction *program, size_t count, size_t fuel,
    UmcResult *result, UmcDiagnostic *diagnostic)
{
    if (result == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    const UmcStatus shape = umc_validate(program, count, diagnostic);
    if (shape != UMC_OK) return shape;
    return execute_validated(program, count, fuel, result, NULL, NULL, diagnostic);
}

UmcStatus umc_execute_trace(const UmcInstruction *program, size_t count, size_t fuel,
    UmcResult *result, UmcTraceStep *steps, size_t capacity, size_t *trace_count,
    UmcDiagnostic *diagnostic)
{
    if (trace_count == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    *trace_count = 0U;
    if (result == NULL || steps == NULL)
        return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    const UmcStatus shape = umc_validate(program, count, diagnostic);
    if (shape != UMC_OK) return shape;
    if (capacity < count) return umc_fail(diagnostic, UMC_BUFFER_TOO_SMALL, 0U);
    return execute_validated(program, count, fuel, result, steps, trace_count, diagnostic);
}
