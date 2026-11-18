/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/source_check.h
 *
 * PURPOSE:
 *   Report bounded source warnings without running code or changing compiler error admission.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


#ifndef UMICOM_C_SOURCE_CHECK_H
#define UMICOM_C_SOURCE_CHECK_H
#include "umicom_c/umicom_c.h"
#ifdef __cplusplus
extern "C" {
#endif

#define UMC_SOURCE_WARNING_PROFILE 1U
/* Every admitted declaration emits a STORE instruction, so the program bound
 * also bounds the total declarations, including disjoint lexical scopes. */
#define UMC_SOURCE_WARNING_LIMIT UMC_PROGRAM_LIMIT

typedef enum UmcSourceWarningKind {
    UMC_WARNING_LOCAL_NEVER_READ = 1
} UmcSourceWarningKind;

typedef struct UmcSourceWarning {
    UmcSourceWarningKind kind;
    size_t source_offset; /* Zero-based byte offset of the declared name. */
    size_t source_length; /* Name bytes, not a retained source pointer. */
    size_t line;          /* One-based byte-oriented location. */
    size_t column;
} UmcSourceWarning;

typedef struct UmcSourceReport {
    size_t instruction_count;
    size_t warning_count;
    UmcSourceWarning warnings[UMC_SOURCE_WARNING_LIMIT];
} UmcSourceReport;

/* Compile and structurally validate through the same owner as umc_compile.
 * No VM execution, allocation, files or callbacks occur. Successful warnings
 * are in declaration order; only warnings[0..warning_count) are meaningful.
 * Plain assignment is a write. Reading in any source path, including an
 * unselected branch, initializer, compound update or increment, counts as a
 * read. This is lexical analysis, not dead-store or path-sensitive analysis.
 * It does not prove arithmetic safety, resource ownership or full C23 support.
 *
 * On error, report remains unchanged; diagnostic follows umc_compile's error
 * contract. Input, report and diagnostic storage must not overlap. The caller
 * owns all storage; no source pointer survives this call. Byte offsets become
 * stale if the caller subsequently edits the source. */
UmcStatus umc_check_source(const char *source, size_t length,
    UmcSourceReport *report, UmcDiagnostic *diagnostic);

#ifdef __cplusplus
}
#endif
#endif
