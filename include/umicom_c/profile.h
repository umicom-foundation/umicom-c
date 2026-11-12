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
