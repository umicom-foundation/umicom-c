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
/* Apostrophes separate digits without changing literal range or wire encoding. */
#define UMC_DIGIT_SEPARATOR_PROFILE 1U
#define UMC_LOCAL_INTEGER_PROFILE 1U
/* Additive source operators and their bounded, forward-only runtime control flow. */
#define UMC_LOGICAL_CONNECTIVE_PROFILE 1U
#define UMC_CONDITIONAL_EXPRESSION_PROFILE 1U
#define UMC_COMPOUND_ASSIGNMENT_PROFILE 1U
#define UMC_FORWARD_BRANCH_PROFILE 1U
/* Checked signed remainder is a source and runtime capability using format 5. */
#define UMC_REMAINDER_PROFILE 1U
/* Signed unary ~ is compiler lowering, not a runtime or wire revision. */
#define UMC_BITWISE_COMPLEMENT_PROFILE 1U
/* Eager signed bitwise operations append format-6 runtime identities. */
#define UMC_BINARY_BITWISE_PROFILE 1U
/* Checked nonnegative int32 shifts append format-7 operations. */
#define UMC_CHECKED_SHIFT_PROFILE 1U
#define UMC_LOCAL_LIMIT 32U
#define UMC_IDENTIFIER_LIMIT 63U
#define UMC_SOURCE_LIMIT 65536U
#define UMC_PROGRAM_LIMIT 256U
#define UMC_STACK_LIMIT 64U
#define UMC_PARSE_DEPTH_LIMIT 64U
#define UMC_BYTECODE_HEADER_SIZE 8U
#define UMC_BYTECODE_RECORD_SIZE 9U
#define UMC_BYTECODE_LIMIT (UMC_BYTECODE_HEADER_SIZE + UMC_PROGRAM_LIMIT * UMC_BYTECODE_RECORD_SIZE)

/* const int and int const initialize one read-only top-level local. All later
 * source updates, including prefix/postfix, are refused even in unselected
 * branches. Initializers may read earlier locals and execute at runtime; this
 * is not constexpr or a new bytecode storage property. The wire and runtime
 * retain ordinary local slots. Qualifiers are not repeated, inferred or applied
 * to the entry function; declaration placement and bounds remain unchanged. */
#define UMC_READONLY_LOCAL_PROFILE 1U

/* The read-only declaration contract above originally limited placement to
 * top-level locals. This additive profile also permits initialized int/const
 * int declarations inside braced if/else bodies. Names cease to be visible at
 * the matching closing brace; disjoint scopes may reuse names and VM slots.
 * At most UMC_LOCAL_LIMIT names are simultaneously visible. Shadowing an
 * active outer name, standalone blocks, early returns and uninitialized
 * declarations remain unsupported. Runtime storage is not erased at scope
 * exit, and source names/qualifiers are not encoded in bytecode. */
#define UMC_BRANCH_LOCAL_PROFILE 1U
/* Standalone blocks now extend the earlier branch-only contract above. They
 * use the same initialized declarations and lexical lifetime, including nested
 * bodies, with one shared parse-depth budget. Braces emit no runtime operation.
 * Active-name shadowing, early returns and uninitialized declarations remain
 * unsupported; leaving a scope neither clears a slot nor frees a resource. */
#define UMC_COMPOUND_STATEMENT_PROFILE 1U
/* Empty statement spelling does not change instruction or wire identities. */
#define UMC_EMPTY_STATEMENT_PROFILE 1U
#define UMC_ASCII_CHARACTER_PROFILE 1U
#define UMC_DECLARATOR_LIST_PROFILE 1U
/* true/false are exact source spellings for integer 1/0, not typed bool locals. */
#define UMC_BOOLEAN_LITERAL_PROFILE 1U

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
    UMC_JUMP, UMC_JUMP_IF_ZERO,
    /* Append format-5 remainder without renumbering formats 1 through 4. */
    UMC_REMAINDER,
    /* Append format 6; formats 1 through 5 keep their opcode identities. */
    UMC_BITWISE_AND, UMC_BITWISE_XOR, UMC_BITWISE_OR,
    /* Append format 7; all earlier identities and structure layouts remain. */
    UMC_SHIFT_LEFT, UMC_SHIFT_RIGHT
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
/* Shift expressions and <<= / >>= statements accept a nonnegative left
 * operand and a count from 0 through 31. Negative inputs/counts, excessive
 * counts and a left-shift result above INT32_MAX return UMC_ARITHMETIC_ERROR
 * during execution without publishing the failed instruction. Right shift
 * divides by a power of two. This checked subset deliberately excludes
 * implementation-dependent signed-negative shifts and unsigned C types.
 * Addition binds more tightly than shifts; shifts bind more tightly than
 * comparisons and associate left to right. Shifts use wire format 7. */

/* Integer tokens accept decimal, 0x/0X hexadecimal and 0b/0B binary, up to
 * INT32_MAX. Unprefixed leading zeroes remain decimal, not C octal. Signs
 * are unary operations; suffixes and unsigned types are excluded. A single
 * apostrophe may separate adjacent digits of the same radix. */
/* Historical restriction before separated spelling: separators were excluded. */
/* Compile: int main(void) { return <signed-32-bit arithmetic expression>; }
 * The output count is zero on failure. Partial instruction bytes may have been
 * written: never execute them after an error. All buffers must be disjoint and
 * valid for their stated capacities. The compiler retains no caller pointers. */
/* Before return, the current grammar admits initialized int declarations and
 * assignments to previously declared locals. Names have at most 63 ASCII bytes;
 * at most 32 slots exist. Initializers cannot read the variable being declared.
 * There are no nested scopes, calls or assignment expressions. */
/* Existing initialized locals also accept +=, -=, *= and /= statements. The
 * old value and right expression each evaluate once; arithmetic is checked by
 * the same runtime used for binary expressions. This adds no opcode or wire
 * version and does not admit assignment expressions or compound declarations. */
/* Historical braced-only conditional contract retained for source review;
 * the following extension admits else-if spelling through the same runtime. */
#if 0
/* Braced if (expression) { updates } and optional else { updates } may
 * precede the final return. Bodies admit assignments and nested if statements
 * but no declarations, early return, standalone scope or else-if shorthand.
 * All names must already be initialized; both arms are compiled. Conditions
 * execute once and zero selects the false arm. Statement and expression
 * nesting share the parse-depth limit; no new opcode or wire format is added. */
#endif
/* Braced if bodies may be followed by else if (expression) { updates }
 * chains and an optional final else { updates }. Conditions are tested in
 * order; only the first selected body executes. Every condition/body parses
 * and references initialized locals. Each nested if, including an else-if,
 * consumes the shared statement/expression depth budget. Bodies still exclude
 * declarations, early returns and standalone scopes. A final top-level return
 * is required. Existing opcode identities and bytecode formats are unchanged. */
/* Binary % binds with * and /, groups from the left and evaluates each
 * operand once. %= updates one initialized local with the same operation.
 * The result follows truncating division and has the dividend's sign when
 * nonzero. A zero divisor and INT32_MIN % -1 fail before host arithmetic.
 * Existing compound-source syntax and result ownership remain unchanged. */
/* Binary &, ^ and | operate eagerly on signed 32-bit representations.
 * Equality binds more tightly than &, then ^, then |, then logical && and ||.
 * Local &=, ^= and |= statements evaluate the old value and right side once.
 * Shifts, address-taking, pointers and assignment expressions are excluded. */
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

/* Earlier minimum-format documentation is retained for compatibility review. */
#if 0
/* Arithmetic-only programs retain format 1. Programs containing comparison
 * opcodes use format 2 when no locals occur. LOAD_LOCAL or STORE_LOCAL requires
 * format 3 when no branches occur. JUMP or JUMP_IF_ZERO requires format 4.
 * Decoding refuses any mismatch with the minimum required profile. */
#endif
/* Retain the format-5 contract for review; format 6 is additive below. */
#if 0
/* Arithmetic uses format 1, comparisons require at least 2, locals at least 3,
 * forward branches at least 4, and remainder at least 5. Encoding selects the
 * maximum requirement across the complete program. A decoder refuses any
 * header version other than that minimum, including a newer unnecessary one.
 * Programs without remainder keep their previous exact wire representation. */
#endif
/* Arithmetic uses format 1, comparisons 2, locals 3, forward branches 4,
 * remainder 5 and binary bitwise operations 6. Select the maximum requirement
 * across all records. Decode rejects both older and unnecessarily newer
 * headers. Programs without binary bitwise operations retain exact old bytes. */
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
