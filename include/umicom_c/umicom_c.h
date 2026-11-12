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
/* Additive arithmetic-profile capability; public struct layouts stay unchanged. */
#define UMC_COMPARISON_PROFILE 1U
/* Source grammar capability; ! lowers to format-2 comparison instructions. */
#define UMC_LOGICAL_NOT_PROFILE 1U
/* Explicit hexadecimal/binary prefixes retain the same signed literal range. */
#define UMC_PREFIXED_INTEGER_PROFILE 1U
#define UMC_LOCAL_INTEGER_PROFILE 1U
/* Additive source operators and their bounded, forward-only runtime control flow. */
#define UMC_LOGICAL_CONNECTIVE_PROFILE 1U
#define UMC_CONDITIONAL_EXPRESSION_PROFILE 1U
#define UMC_FORWARD_BRANCH_PROFILE 1U
#define UMC_LOCAL_LIMIT 32U
#define UMC_IDENTIFIER_LIMIT 63U
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
    UMC_BUFFER_TOO_SMALL,
    UMC_LOCAL_CAPACITY
} UmcStatus;

typedef enum UmcOpcode {
    UMC_PUSH = 1,
    UMC_ADD,
    UMC_SUBTRACT,
    UMC_MULTIPLY,
    UMC_DIVIDE,
    UMC_NEGATE,
    UMC_RETURN,
    /* Append wire identities; never renumber arithmetic format-1 opcodes. */
    UMC_LESS, UMC_LESS_EQUAL, UMC_GREATER, UMC_GREATER_EQUAL,
    UMC_EQUAL, UMC_NOT_EQUAL,
    /* Format 3 slots are bounded integers, never native addresses. */
    UMC_LOAD_LOCAL, UMC_STORE_LOCAL,
    /* Format 4 uses absolute forward instruction indices, never native addresses. */
    UMC_JUMP, UMC_JUMP_IF_ZERO
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

/* The comparison extension also accepts <, <=, >, >=, == and != within the
 * expression. Results are int32 zero or one; both operands execute. The original
 * arithmetic form below remains accepted without changing its wire encoding. */
/* Unary ! evaluates its operand then compares it with zero. It uses two
 * additional instructions and retains all normal source/depth/stack limits. */
/* Integer tokens accept decimal, 0x/0X hexadecimal and 0b/0B binary, up to
 * INT32_MAX. Unprefixed leading zeroes remain decimal, not C octal. Signs
 * are unary operations; suffixes, separators and unsigned types are excluded. */
/* Compile: int main(void) { return <signed-32-bit arithmetic expression>; }
 * The output count is zero on failure. Partial instruction bytes may have been
 * written: never execute them after an error. All buffers must be disjoint and
 * valid for their stated capacities. The compiler retains no caller pointers. */
/* Before return, the current grammar admits initialized int declarations and
 * assignments to previously declared locals. Names have at most 63 ASCII bytes;
 * at most 32 slots exist. Initializers cannot read the variable being declared.
 * There are no nested scopes, calls or assignment expressions. */
UmcStatus umc_compile(const char *source, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count,
    UmcDiagnostic *diagnostic);

/* Validation checks the entire program, including bytes after RETURN, before
 * execution begins. The original profile has no branch, memory or host-call
 * instructions. The local extension adds only bounded per-execution integer
 * slots; every LOAD must follow a STORE. No native addresses are exposed.
 * The forward-branch extension checks all structural paths, equal stack depth
 * at joins, and stores on every incoming path before each local read. */
UmcStatus umc_validate(const UmcInstruction *program, size_t count,
    UmcDiagnostic *diagnostic);

/* Originally these facts described straight-line bytecode. They now count
 * all admitted records, including alternative paths; peak_stack is the maximum
 * over those paths. Counts are not a guarantee that arithmetic will succeed. */
typedef struct UmcProgramInfo {
    size_t instruction_count;
    size_t peak_stack;
    size_t constant_count;
    size_t arithmetic_count;
    size_t bytecode_size;
} UmcProgramInfo;

/* Inspect without executing. The same complete validation used by execution
 * supplies these facts. Output is unchanged on failure; it must be disjoint
 * from the program and optional diagnostic. Program memory is borrowed and
 * must remain immutable throughout the call. Successful execution of this
 * branch-free profile needs instruction_count fuel, including RETURN. Forward
 * branches make that count a sufficient upper bound; skipped records consume
 * no fuel and successful execution can require fewer instructions. */
UmcStatus umc_inspect(const UmcInstruction *program, size_t count,
    UmcProgramInfo *info, UmcDiagnostic *diagnostic);

/* Fuel counts executed instructions including RETURN. Validation work is
 * separately bounded by UMC_PROGRAM_LIMIT. Result is unchanged on failure.
 * Caller-owned program memory must remain immutable for the duration of the call. */
UmcStatus umc_execute(const UmcInstruction *program, size_t count, size_t fuel,
    UmcResult *result, UmcDiagnostic *diagnostic);

/* Arithmetic-only programs retain format 1. Programs containing comparison
 * opcodes use format 2 when no locals occur. LOAD_LOCAL or STORE_LOCAL requires
 * format 3 when no branches occur. JUMP or JUMP_IF_ZERO requires format 4.
 * Decoding refuses any mismatch with the minimum required profile. */
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
