/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/execution_bounds.c
 *
 * PURPOSE:
 *   Reuse complete bytecode validation before reverse propagation of structural path costs.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "internal.h"
#include "umicom_c/execution_bounds.h"

UmcStatus umc_execution_bounds(const UmcInstruction *program, size_t count,
    UmcExecutionBounds *bounds, UmcDiagnostic *diagnostic)
{
    if (bounds == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    const UmcStatus status = umc_validate(program, count, diagnostic);
    if (status != UMC_OK) return status;
    size_t minimum[UMC_PROGRAM_LIMIT];
    size_t maximum[UMC_PROGRAM_LIMIT];
    /* Validation guarantees a final RETURN, valid forward targets and complete
     * reachability. Every successor is therefore already measured in this
     * reverse walk; no recursion, work queue or repeated fixed point is needed. */
    for (size_t position = count; position != 0U;) {
        const size_t index = --position;
        const UmcInstruction *instruction = &program[index];
        if (instruction->opcode == UMC_RETURN) {
            minimum[index] = 1U;
            maximum[index] = 1U;
            continue;
        }
        const size_t next = instruction->opcode == UMC_JUMP
            ? (size_t)instruction->operand : index + 1U;
        size_t low = minimum[next];
        size_t high = maximum[next];
        if (instruction->opcode == UMC_JUMP_IF_ZERO) {
            const size_t target = (size_t)instruction->operand;
            if (minimum[target] < low) low = minimum[target];
            if (maximum[target] > high) high = maximum[target];
        }
        /* Forward-only paths visit each record at most once, so these sums
         * cannot exceed the already admitted program count. */
        minimum[index] = low + 1U;
        maximum[index] = high + 1U;
    }
    bounds->minimum_instructions = minimum[0];
    bounds->maximum_instructions = maximum[0];
    return UMC_OK;
}
