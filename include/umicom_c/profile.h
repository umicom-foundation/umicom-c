/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/profile.h
 *
 * PURPOSE:
 *   Query capabilities and bounds of the linked compiler and runtime without caller-owned storage.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMC_PROFILE_H
#define UMC_PROFILE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Stable query IDs and feature bits are appended, never renumbered. A zero
 * result means unknown/not provided by this component. These are declarations
 * of the linked implementation, not an authenticated build identity. */
typedef enum UmcProfileProperty {
    UMC_PROFILE_API = 1,
    UMC_PROFILE_FEATURES,
    UMC_PROFILE_SOURCE_BYTES,
    UMC_PROFILE_PROGRAM_INSTRUCTIONS,
    UMC_PROFILE_STACK_VALUES,
    UMC_PROFILE_PARSE_DEPTH,
    UMC_PROFILE_BYTECODE_BYTES,
    UMC_PROFILE_WIRE_VERSIONS,
    UMC_PROFILE_LOCAL_VALUES,
    UMC_PROFILE_IDENTIFIER_BYTES
} UmcProfileProperty;

#define UMC_FEATURE_ARITHMETIC UINT32_C(1)
#define UMC_FEATURE_COMPARISON UINT32_C(2)
#define UMC_FEATURE_LOGICAL_NOT UINT32_C(4)
#define UMC_FEATURE_EXECUTION_TRACE UINT32_C(8)
#define UMC_FEATURE_PROGRAM_INSPECTION UINT32_C(16)
/* Source-only spelling features; their values still use ordinary PUSH. */
#define UMC_FEATURE_HEXADECIMAL_LITERALS UINT32_C(32)
#define UMC_FEATURE_BINARY_LITERALS UINT32_C(64)
/* Compiler grammar and runtime slot operations share this implemented capability. */
#define UMC_FEATURE_LOCAL_INTEGERS UINT32_C(128)
/* Distinguish runtime graph admission from source logical operator spelling. */
#define UMC_FEATURE_FORWARD_BRANCHES UINT32_C(256)
#define UMC_FEATURE_LOGICAL_CONNECTIVES UINT32_C(512)
/* Conditional values lower to the existing forward branch runtime. */
#define UMC_FEATURE_CONDITIONAL_EXPRESSIONS UINT32_C(1024)
/* Local += -= *= /= statements reuse load, arithmetic and store records. */
#define UMC_FEATURE_COMPOUND_ASSIGNMENTS UINT32_C(2048)
/* Braced if/else updates reuse existing checked forward-branch records. */
#define UMC_FEATURE_IF_STATEMENTS UINT32_C(4096)
/* Else-if source chains reuse format 4 without a runtime operation change. */
#define UMC_FEATURE_ELSE_IF_CHAINS UINT32_C(8192)
/* Binary remainder and %= share this compiler/runtime arithmetic capability. */
#define UMC_FEATURE_REMAINDER UINT32_C(16384)
/* Signed unary complement lowers to existing arithmetic; compiler only. */
#define UMC_FEATURE_BITWISE_COMPLEMENT UINT32_C(32768)
/* Eager binary bitwise source and runtime support requires wire format 6. */
#define UMC_FEATURE_BINARY_BITWISE UINT32_C(65536)
/* Standalone initialized-local ++/-- lowers to existing slot/arithmetic records. */
#define UMC_FEATURE_INCREMENT_STATEMENTS UINT32_C(131072)
/* Source spelling only; the runtime continues to execute ordinary PUSH values. */
#define UMC_FEATURE_DIGIT_SEPARATORS UINT32_C(262144)
/* Source qualifier and write admission only; no immutable runtime storage. */
#define UMC_FEATURE_READONLY_LOCALS UINT32_C(524288)
/* Initialized names scoped to if/else bodies; existing local/branch bytecode. */
#define UMC_FEATURE_BRANCH_LOCALS UINT32_C(1048576)
/* Standalone braced source scopes use existing initialized local operations. */
#define UMC_FEATURE_COMPOUND_STATEMENTS UINT32_C(2097152)
/* Optional lexical source report; no execution or runtime warning state. */
#define UMC_FEATURE_SOURCE_WARNINGS UINT32_C(4194304)
/* Checked nonnegative shift expressions/runtime operations use format 7. */
#define UMC_FEATURE_CHECKED_SHIFTS UINT32_C(8388608)
/* Standalone semicolons are source grammar only; no runtime NOP is emitted. */
#define UMC_FEATURE_EMPTY_STATEMENTS UINT32_C(16777216)
/* ASCII character spelling lowers to integer PUSH; compiler only. */
#define UMC_FEATURE_ASCII_CHARACTER_LITERALS UINT32_C(33554432)
/* Initialized declaration lists change grammar, not runtime operations. */
#define UMC_FEATURE_DECLARATOR_LISTS UINT32_C(67108864)
/* Exact true/false source literals lower to integer 1/0; no bool local type. */
#define UMC_FEATURE_BOOLEAN_LITERALS UINT32_C(134217728)

/* Runtime reports arithmetic/comparison, trace and inspection, program/stack/
 * bytecode limits, and supported wire-version bits (1U << version).
 * Compiler reports source grammar features and source/program/depth limits.
 * Neither reports the other's component-specific operations. No allocator,
 * mutable global state, host service or C-structure ABI is involved.
 *
 * These are additive symbols. Older libraries without them fail to link;
 * a new header alone cannot retrofit a query into an old binary. */
uint32_t umc_runtime_profile(UmcProfileProperty property);
uint32_t umc_compiler_profile(UmcProfileProperty property);

#ifdef __cplusplus
}
#endif
#endif
