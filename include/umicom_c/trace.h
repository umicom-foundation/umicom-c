/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/trace.h
 *
 * PURPOSE:
 *   Describe bounded arithmetic execution using caller-owned storage without host callbacks.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMICOM_C_TRACE_H
#define UMICOM_C_TRACE_H
#include "umicom_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A successful step describes the stack after its instruction. A failed step
 * describes the unchanged stack before the rejected instruction; fuel exhaustion
 * records the next instruction without executing it. top is zero at depth zero.
 * This is a compact arithmetic trace, not a full stack snapshot or native debugger. */
typedef struct UmcTraceStep {
    size_t instruction_index;
    size_t source_offset;
    UmcOpcode opcode;
    size_t stack_depth;
    int32_t top;
    UmcStatus status;
} UmcTraceStep;

/* Validate the entire program, then require capacity >= count before execution.
 * The caller owns steps and keeps all input/output regions disjoint. trace_count
 * is required and cleared on entry. Invalid arguments, invalid bytecode and
 * insufficient storage leave steps and result unchanged with trace_count zero.
 * A runtime fault retains completed steps plus one failed attempted step; result
 * stays unchanged. Success includes RETURN. No allocation, callback, host I/O or
 * bytecode-format change is introduced by tracing itself. fuel may be zero.
 * With forward branches, rows retain original instruction indices and omit
 * skipped records. Capacity still covers count; trace_count counts actual rows. */
UmcStatus umc_execute_trace(const UmcInstruction *program, size_t count, size_t fuel,
    UmcResult *result, UmcTraceStep *steps, size_t capacity, size_t *trace_count,
    UmcDiagnostic *diagnostic);

#ifdef __cplusplus
}
#endif
#endif
