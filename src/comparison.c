/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/comparison.c
 *
 * PURPOSE:
 *   Compare validated operations separately from diagnostic source locations.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "umicom_c/comparison.h"
#include "internal.h"
UmcStatus umc_program_same_operations(const UmcInstruction *left, size_t left_count,
    const UmcInstruction *right, size_t right_count, bool *equal, UmcDiagnostic *diagnostic)
{
    if (equal == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    *equal = false;
    UmcStatus status = umc_validate(left, left_count, diagnostic);
    if (status != UMC_OK) return status;
    status = umc_validate(right, right_count, diagnostic);
    if (status != UMC_OK) return status;
    if (left_count != right_count) return UMC_OK;
    for (size_t i = 0U; i < left_count; ++i) {
        if (left[i].opcode != right[i].opcode || left[i].operand != right[i].operand)
            return UMC_OK;
    }
    *equal = true;
    return UMC_OK;
}
