/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/int_input.h
 *
 * PURPOSE:
 *   Compile and bind a checked entry point with one host-supplied int32 value.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMICOM_C_INT_INPUT_H
#define UMICOM_C_INT_INPUT_H

#include "umicom_c/umicom_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UMC_INT_INPUT_BYTECODE_FLAG 1U

/* Compile int umicom_run(int parameter) with the existing bounded statements
 * and final return. The parameter occupies the first initialized local slot.
 * The artifact uses the normal instruction version and a required-input flag
 * in header byte 5. The flag keeps ordinary decoders from executing a zero
 * placeholder by mistake. On failure *size is zero and bytes stay unchanged.
 * Keep source, output, size and diagnostic storage disjoint. */
UmcStatus umc_compile_int_input(const char *source, size_t length, uint8_t *output,
    size_t capacity, size_t *size, UmcDiagnostic *diagnostic);

/* Validate an entire required-input artifact, then bind one signed integer to
 * its initial local. The output is an ordinary runnable instruction sequence.
 * All bindings preserve the input artifact. Failure clears *count and leaves
 * instruction storage unchanged. Use one output per concurrent call. Keep the
 * input and all outputs disjoint. No host addresses are exposed to the guest. */
UmcStatus umc_bind_int_input_bytecode(const uint8_t *bytes, size_t length, int32_t value,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic);

#ifdef __cplusplus
}
#endif
#endif
