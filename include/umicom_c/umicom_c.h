/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/umicom_c.h
 *
 * PURPOSE:
 *   Define bounded compiler, bytecode and execution contracts without a host dependency.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#ifndef UMICOM_C_H
#define UMICOM_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UMC_API_VERSION 1U
#define UMC_SOURCE_LIMIT 65536U
#define UMC_PROGRAM_LIMIT 256U
#define UMC_STACK_LIMIT 64U
#define UMC_PARSE_DEPTH_LIMIT 64U
#define UMC_BYTECODE_HEADER_SIZE 8U
#define UMC_BYTECODE_RECORD_SIZE 9U
#define UMC_BYTECODE_LIMIT (UMC_BYTECODE_HEADER_SIZE + UMC_PROGRAM_LIMIT * UMC_BYTECODE_RECORD_SIZE)

typedef enum UmcStatus {
    UMC_OK = 0,
    UMC_INVALID_ARGUMENT,
    UMC_SOURCE_TOO_LARGE,
    UMC_SYNTAX_ERROR,
    UMC_LITERAL_RANGE,
    UMC_PROGRAM_TOO_LARGE,
    UMC_DEPTH_LIMIT,
    UMC_BAD_BYTECODE,
    UMC_STACK_OVERFLOW,
    UMC_FUEL_EXHAUSTED,
    UMC_ARITHMETIC_ERROR,
    UMC_BUFFER_TOO_SMALL
} UmcStatus;

typedef enum UmcOpcode {
    UMC_PUSH = 1,
    UMC_ADD,
    UMC_SUBTRACT,
    UMC_MULTIPLY,
    UMC_DIVIDE,
    UMC_NEGATE,
    UMC_RETURN
} UmcOpcode;

typedef struct UmcInstruction {
    UmcOpcode opcode;
    int32_t operand;
    size_t source_offset;
} UmcInstruction;

typedef struct UmcDiagnostic {
    UmcStatus status;
    size_t source_offset;
    size_t line;
    size_t column;
    const char *message; /* Static storage; never free this pointer. */
} UmcDiagnostic;

typedef struct UmcResult {
    int32_t value;
    size_t instructions_executed;
} UmcResult;

const char *umc_status_text(UmcStatus status);
const char *umc_opcode_text(UmcOpcode opcode);

/* The diagnostic is optional. Location is byte-based, one-based, with CRLF
 * counted as one line break. Source is a borrowed span, not necessarily a C string. */
void umc_locate_diagnostic(const char *source, size_t length, UmcDiagnostic *diagnostic);

/* Compile: int main(void) { return <signed-32-bit arithmetic expression>; }
 * The output count is zero on failure. Partial instruction bytes may have been
 * written: never execute them after an error. All buffers must be disjoint and
 * valid for their stated capacities. The compiler retains no caller pointers. */
UmcStatus umc_compile(const char *source, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count,
    UmcDiagnostic *diagnostic);

/* Validation checks the entire program, including bytes after RETURN, before
 * execution begins. Programs have no branch, memory or host-call instructions. */
UmcStatus umc_validate(const UmcInstruction *program, size_t count,
    UmcDiagnostic *diagnostic);

/* Fuel counts executed instructions including RETURN. Validation work is
 * separately bounded by UMC_PROGRAM_LIMIT. Result is unchanged on failure.
 * Caller-owned program memory must remain immutable for the duration of the call. */
UmcStatus umc_execute(const UmcInstruction *program, size_t count, size_t fuel,
    UmcResult *result, UmcDiagnostic *diagnostic);

/* Stable little-endian bytecode. No structure casts, executable mappings, host
 * addresses or machine-dependent padding are accepted. Size/count is zero on
 * failure; buffers may contain a partial decode and must not then be executed. */
UmcStatus umc_encode(const UmcInstruction *program, size_t count,
    uint8_t *output, size_t capacity, size_t *size, UmcDiagnostic *diagnostic);
UmcStatus umc_decode(const uint8_t *bytes, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic);

#ifdef __cplusplus
}
#endif
#endif
