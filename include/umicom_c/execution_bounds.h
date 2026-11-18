/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/execution_bounds.h
 *
 * PURPOSE:
 *   Plan structural instruction bounds without executing the validated forward-only bytecode graph.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMICOM_C_EXECUTION_BOUNDS_H
#define UMICOM_C_EXECUTION_BOUNDS_H
#include "umicom_c/umicom_c.h"
#ifdef __cplusplus
extern "C" {
#endif
#define UMC_EXECUTION_BOUNDS_PROFILE 1U

typedef struct UmcExecutionBounds {
    size_t minimum_instructions;
    size_t maximum_instructions;
} UmcExecutionBounds;

/* Validate through the runtime's complete graph owner, then count the shortest
 * and longest STRUCTURAL paths from entry through RETURN, counting both ends.
 * Both conditional edges are considered even when values make one infeasible.
 * No constants, arithmetic, local values or branch conditions are executed.
 *
 * Any successful fresh execution lies within these bounds. The maximum is a
 * sufficient fuel allowance, not a promise of arithmetic success; a runtime
 * fault can occur before the minimum. This is not a time or memory estimate.
 * Existing program inspection, wire profiles and runtime semantics are unchanged.
 *
 * On failure bounds remains unchanged. Diagnostic follows umc_validate, except
 * a null bounds pointer is INVALID_ARGUMENT. All objects must be valid and
 * disjoint. Program storage stays immutable for the call; no pointer is retained.
 * Fixed automatic storage is used without allocation, callbacks or host I/O. */
UmcStatus umc_execution_bounds(const UmcInstruction *program, size_t count,
    UmcExecutionBounds *bounds, UmcDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
