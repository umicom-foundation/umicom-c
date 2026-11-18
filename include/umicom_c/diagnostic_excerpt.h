/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/diagnostic_excerpt.h
 *
 * PURPOSE:
 *   Describe a bounded ASCII rendering of a borrowed source line without terminal control bytes.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMICOM_C_DIAGNOSTIC_EXCERPT_H
#define UMICOM_C_DIAGNOSTIC_EXCERPT_H
#include "umicom_c/umicom_c.h"
#define UMC_EXCERPT_SOURCE_BYTES 80U
#define UMC_EXCERPT_TEXT_BYTES (4U * UMC_EXCERPT_SOURCE_BYTES + 7U)
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UmcDiagnosticExcerpt {
    char text[UMC_EXCERPT_TEXT_BYTES];
    size_t caret_column; /* Zero-based ASCII display column, excluding a caller prefix. */
    size_t line;         /* One-based source line. */
    size_t column;       /* One-based source byte column, not a Unicode glyph column. */
    size_t source_begin;
    size_t source_end;   /* Exclusive source window end. */
    int clipped_left;
    int clipped_right;
} UmcDiagnosticExcerpt;

/* Source is a stable borrowed span; it need not end with NUL. All source bytes,
 * including NUL, are data. Output must be valid and disjoint from that span.
 * Reject NULL, length above UMC_SOURCE_LIMIT or offset above length without
 * changing output. Offset == length denotes EOF. A position between CR and LF
 * selects the following line, matching umc_locate_diagnostic's location.
 *
 * Successful text contains printable ASCII only and a final NUL: backslash is
 * doubled, tab becomes backslash-t, other nonprinting/non-ASCII bytes become
 * backslash-x followed by two uppercase hexadecimal digits. At most 80 source
 * bytes are rendered, with three dots at each clipped edge. The caret is at
 * the escaped representation's beginning, or one past it for line-end/EOF.
 * No allocation, terminal I/O, Unicode decoding or source mutation occurs. */
UmcStatus umc_diagnostic_excerpt(const char *source, size_t length,
    size_t offset, UmcDiagnosticExcerpt *output);
#ifdef __cplusplus
}
#endif
#endif
