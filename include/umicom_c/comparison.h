/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/comparison.h
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
#ifndef UMICOM_C_COMPARISON_H
#define UMICOM_C_COMPARISON_H
#include "umicom_c/umicom_c.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Compare validated instruction identity, ignoring diagnostic source offsets.
 * Equal means the same opcodes and operands, not equivalence of arbitrary C.
 * This does not authorize reusing builds: compiler, flags, dependencies and
 * diagnostics still have their own identities. Failure always clears equal. */
UmcStatus umc_program_same_operations(const UmcInstruction *left, size_t left_count,
    const UmcInstruction *right, size_t right_count, bool *equal, UmcDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
