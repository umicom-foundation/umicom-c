/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/source_regions.h
 *
 * PURPOSE:
 *   Describe explicit language islands without claiming frontend or cross-language execution support.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMICOM_C_SOURCE_REGIONS_H
#define UMICOM_C_SOURCE_REGIONS_H
#include "umicom_c.h"
#ifdef __cplusplus
extern "C" {
#endif
#define UMC_REGION_LIMIT 64U
/* Envelope admission bound; not a FreeBASIC implementation limit. */
#define UMC_BASIC_COMMENT_DEPTH_LIMIT 32U
/* Envelope bound only; not a D compiler limit. */
#define UMC_D_COMMENT_DEPTH_LIMIT 32U
/* Preserve the original four-language declaration while appending the D classification identity. The former implementation is retained for owner review. */
#if 0
typedef enum UmcSourceLanguage { UMC_LANGUAGE_UMICOM,UMC_LANGUAGE_C23,UMC_LANGUAGE_BASIC,UMC_LANGUAGE_JAVA } UmcSourceLanguage;
#endif
/* Append D so existing numeric language identities stay stable. */
typedef enum UmcSourceLanguage { UMC_LANGUAGE_UMICOM,UMC_LANGUAGE_C23,UMC_LANGUAGE_BASIC,UMC_LANGUAGE_JAVA,UMC_LANGUAGE_D } UmcSourceLanguage;
typedef struct UmcSourceRegion {
    UmcSourceLanguage language;
    size_t offset,length,tag_offset;
} UmcSourceRegion;
const char *umc_source_language_name(UmcSourceLanguage language);
/* Earlier boundary contract retained for review: BASIC REM was not implemented.
 * The additive scanner below now accepts only the explicitly described form. */
/* Partition a borrowed bounded source span. Explicit tags are lowercase
 * <?umicom, <?c23, <?basic and <?java, followed by whitespace, ending with ?>.
 * Untagged text uses Umicom C. Tags cannot nest. Regions exclude tag bytes;
 * ordinary C-style quoted strings and comments protect delimiters. BASIC
 * supports doubled quotes, apostrophe comments and case-insensitive REM at a
 * statement start (line start or after a colon, allowing whitespace). REM
 * requires whitespace or the end of the source span after its three letters.
 * BASIC block comments use slash-apostrophe and apostrophe-slash delimiters,
 * nest to UMC_BASIC_COMMENT_DEPTH_LIMIT, and preserve the enclosing statement
 * across internal newlines. Quotes and language tags inside them are inert;
 * nested comment markers are still recognised inside apparent quoted text.
 * These rules protect delimiters for boundary scanning only.
 * This is an experimental lexical envelope, not a C23/BASIC/Java parser:
 * Java Unicode preprocessing/text blocks and BASIC line continuation are not
 * implemented. BASIC metacommands are not interpreted. The former contract
 * excluded BASIC block comments; the bounded form above replaces that limit.
 * Use separate source files for those forms. No region is executed or compiled.
 * Output count is zero on failure; partial region bytes must not then be used.
 * All regions must be disjoint, valid storage; output contains no borrowed pointer. */
/* D boundary extension: <?d plus whitespace admits ordinary quoted strings
 * and characters, r-prefixed double-quoted and backtick raw strings, // and
 * non-nesting C-style block comments, and /+ +/ comments nesting at most
 * UMC_D_COMMENT_DEPTH_LIMIT. No escape is interpreted inside a raw string.
 * Apparent quotes do not hide nested comment markers inside /+ +/.
 * q-delimited/token strings, x-hex strings and i-prefixed interpolation are
 * explicitly refused at their prefix. Use separate D files for those forms.
 * This extends the tag list above without claiming a D parser, Unicode
 * preprocessing, compilation, execution or mixed-language linking.
 */
UmcStatus umc_source_regions(const char *source,size_t length,UmcSourceRegion *regions,
    size_t capacity,size_t *count,UmcDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
