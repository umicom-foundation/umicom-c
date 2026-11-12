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

_Static_assert(UMC_LOCAL_LIMIT == 32U, "Review definite-initialization bitmap when changing local bounds");

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

/* Admit every local index and prove a preceding store before each load in the complete straight-line program. The predecessor remains disabled for review. */
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

#endif

/* Replace the linear local proof with bounded forward-graph admission, preserving whole-program checks and unchanged outputs on failure. The predecessor remains disabled for review. */
#if 0
static UmcStatus inspect_program(const UmcInstruction *program, size_t count,
    UmcProgramInfo *info, UmcDiagnostic *diagnostic)
{
    (void)umc_fail(diagnostic, UMC_OK, 0U);
    if (program == NULL || count == 0U || count > UMC_PROGRAM_LIMIT) {
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    }
    uint32_t initialized = 0U;
    size_t depth = 0U;
    size_t peak = 0U;
    size_t constants = 0U;
    size_t arithmetic = 0U;
    for (size_t i = 0U; i < count; ++i) {
        const UmcInstruction *instruction = &program[i];
        const size_t offset = instruction->source_offset;
        if (offset > UMC_SOURCE_LIMIT ||
            (instruction->opcode != UMC_PUSH && instruction->opcode != UMC_LOAD_LOCAL &&
             instruction->opcode != UMC_STORE_LOCAL && instruction->operand != 0)) {
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        }
        if ((instruction->opcode == UMC_LOAD_LOCAL || instruction->opcode == UMC_STORE_LOCAL) &&
            (instruction->operand < 0 || (uint32_t)instruction->operand >= UMC_LOCAL_LIMIT))
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        switch (instruction->opcode) {
        case UMC_LOAD_LOCAL:
            if (!(initialized & (UINT32_C(1) << (uint32_t)instruction->operand)))
                return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            if (depth == UMC_STACK_LIMIT)
                return umc_fail(diagnostic, UMC_STACK_OVERFLOW, offset);
            ++depth;
            if (depth > peak) peak = depth;
            break;
        case UMC_STORE_LOCAL:
            if (depth == 0U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            initialized |= UINT32_C(1) << (uint32_t)instruction->operand;
            break;
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

#endif

/* Forward-only edges make instruction order a topological order. Every incoming
 * state is known before a target is visited. A join keeps only locals written on
 * all paths and requires one unambiguous stack depth. UINT8_MAX means unseen;
 * the actual depth is bounded below that sentinel by UMC_STACK_LIMIT. */
_Static_assert(UMC_STACK_LIMIT < UINT8_MAX, "Stack depth must fit the admission state");
static int merge_state(uint8_t *depths, uint32_t *initialized, size_t target,
    size_t depth, uint32_t local_bits)
{
    if (depths[target] == UINT8_MAX) {
        depths[target] = (uint8_t)depth;
        initialized[target] = local_bits;
        return 1;
    }
    if (depths[target] != depth) return 0;
    initialized[target] &= local_bits;
    return 1;
}
static UmcStatus inspect_program(const UmcInstruction *program, size_t count,
    UmcProgramInfo *info, UmcDiagnostic *diagnostic)
{
    (void)umc_fail(diagnostic, UMC_OK, 0U);
    if (program == NULL || count == 0U || count > UMC_PROGRAM_LIMIT)
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    uint8_t depths[UMC_PROGRAM_LIMIT];
    uint32_t local_states[UMC_PROGRAM_LIMIT];
    for (size_t i = 0U; i < count; ++i) {
        depths[i] = UINT8_MAX;
        local_states[i] = 0U;
    }
    depths[0] = 0U;
    size_t peak = 0U, constants = 0U, arithmetic = 0U;
    for (size_t i = 0U; i < count; ++i) {
        const UmcInstruction *instruction = &program[i];
        const size_t offset = instruction->source_offset;
        const bool local = instruction->opcode == UMC_LOAD_LOCAL || instruction->opcode == UMC_STORE_LOCAL;
        const bool branch = instruction->opcode == UMC_JUMP || instruction->opcode == UMC_JUMP_IF_ZERO;
        if (offset > UMC_SOURCE_LIMIT || depths[i] == UINT8_MAX ||
            (instruction->opcode != UMC_PUSH && !local && !branch && instruction->operand != 0))
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        if (local && (instruction->operand < 0 || (uint32_t)instruction->operand >= UMC_LOCAL_LIMIT))
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        if (branch && (instruction->operand < 0 || (size_t)instruction->operand <= i ||
            (size_t)instruction->operand >= count))
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        size_t depth = depths[i];
        uint32_t initialized = local_states[i];
        switch (instruction->opcode) {
        case UMC_LOAD_LOCAL:
            if (!(initialized & (UINT32_C(1) << (uint32_t)instruction->operand)))
                return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            if (depth == UMC_STACK_LIMIT) return umc_fail(diagnostic, UMC_STACK_OVERFLOW, offset);
            ++depth;
            break;
        case UMC_STORE_LOCAL:
            if (depth == 0U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            initialized |= UINT32_C(1) << (uint32_t)instruction->operand;
            break;
        case UMC_PUSH:
            if (depth == UMC_STACK_LIMIT) return umc_fail(diagnostic, UMC_STACK_OVERFLOW, offset);
            ++depth;
            ++constants;
            break;
        case UMC_ADD: case UMC_SUBTRACT: case UMC_MULTIPLY: case UMC_DIVIDE:
            if (depth < 2U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            ++arithmetic;
            break;
        case UMC_LESS: case UMC_LESS_EQUAL: case UMC_GREATER:
        case UMC_GREATER_EQUAL: case UMC_EQUAL: case UMC_NOT_EQUAL:
            if (depth < 2U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            /* Comparisons and branches are not arithmetic_count entries. */
            break;
        case UMC_NEGATE:
            if (depth == 0U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            ++arithmetic;
            break;
        case UMC_JUMP_IF_ZERO:
            if (depth == 0U) return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            --depth;
            break;
        case UMC_JUMP:
            break;
        case UMC_RETURN:
            if (depth != 1U || i + 1U != count)
                return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
            break;
        default:
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        }
        if (depth > peak) peak = depth;
        if (branch && !merge_state(depths, local_states, (size_t)instruction->operand, depth, initialized))
            return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        if (instruction->opcode != UMC_JUMP && instruction->opcode != UMC_RETURN) {
            if (i + 1U == count || !merge_state(depths, local_states, i + 1U, depth, initialized))
                return umc_fail(diagnostic, UMC_BAD_BYTECODE, offset);
        }
    }
    if (program[count - 1U].opcode != UMC_RETURN)
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, program[count - 1U].source_offset);
    /* Structural counts include both alternatives. Forward edges ensure no
     * execution visits more than count records; counts are not a path trace. */
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

/* Execute admitted local slots in the same engine used by ordinary and traced callers. The predecessor remains disabled for review. */
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

#endif

/* Follow admitted forward targets while charging fuel and recording trace rows only for attempted instructions; skipped operands have no runtime effect. The predecessor remains disabled for review. */
#if 0
static UmcStatus execute_validated(const UmcInstruction *program, size_t count,
    size_t fuel, UmcResult *result, UmcTraceStep *steps, size_t *trace_count,
    UmcDiagnostic *diagnostic)
{
    int32_t stack[UMC_STACK_LIMIT];
    /* Complete validation proves each read follows a write. This automatic
     * frame belongs to one execution and exposes no address or persistent state. */
    int32_t locals[UMC_LOCAL_LIMIT];
    size_t depth = 0U;
    for (size_t pc = 0U; pc < count; ++pc) {
        const UmcInstruction *instruction = &program[pc];
        UmcStatus status = UMC_OK;
        if (pc >= fuel) {
            status = UMC_FUEL_EXHAUSTED;
        } else if (instruction->opcode == UMC_PUSH) {
            stack[depth++] = instruction->operand;
        } else if (instruction->opcode == UMC_LOAD_LOCAL) {
            stack[depth++] = locals[(size_t)instruction->operand];
        } else if (instruction->opcode == UMC_STORE_LOCAL) {
            locals[(size_t)instruction->operand] = stack[--depth];
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

#endif

static UmcStatus execute_validated(const UmcInstruction *program, size_t count,
    size_t fuel, UmcResult *result, UmcTraceStep *steps, size_t *trace_count,
    UmcDiagnostic *diagnostic)
{
    int32_t stack[UMC_STACK_LIMIT];
    /* Complete validation proves each read follows a write. This automatic
     * frame belongs to one execution and exposes no address or persistent state. */
    int32_t locals[UMC_LOCAL_LIMIT];
    size_t depth = 0U;
    for (size_t pc = 0U, executed = 0U; pc < count; ++executed) {
        size_t next = pc + 1U;
        const UmcInstruction *instruction = &program[pc];
        UmcStatus status = UMC_OK;
        if (executed >= fuel) {
            status = UMC_FUEL_EXHAUSTED;
        } else if (instruction->opcode == UMC_PUSH) {
            stack[depth++] = instruction->operand;
        } else if (instruction->opcode == UMC_LOAD_LOCAL) {
            stack[depth++] = locals[(size_t)instruction->operand];
        } else if (instruction->opcode == UMC_STORE_LOCAL) {
            locals[(size_t)instruction->operand] = stack[--depth];
        } else if (instruction->opcode == UMC_JUMP) {
            next = (size_t)instruction->operand;
        } else if (instruction->opcode == UMC_JUMP_IF_ZERO) {
            if (stack[--depth] == 0) next = (size_t)instruction->operand;
        } else if (instruction->opcode == UMC_RETURN) {
            record_step(steps, trace_count, pc, instruction, stack, depth, UMC_OK);
            result->value = stack[0];
            result->instructions_executed = executed + 1U;
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
        pc = next;
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
