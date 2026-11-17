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
typedef enum UmcSourceLanguage { UMC_LANGUAGE_UMICOM,UMC_LANGUAGE_C23,UMC_LANGUAGE_BASIC,UMC_LANGUAGE_JAVA } UmcSourceLanguage;
typedef struct UmcSourceRegion {
    UmcSourceLanguage language;
    size_t offset,length,tag_offset;
} UmcSourceRegion;
const char *umc_source_language_name(UmcSourceLanguage language);
/* Partition a borrowed bounded source span. Explicit tags are lowercase
 * <?umicom, <?c23, <?basic and <?java, followed by whitespace, ending with ?>.
 * Untagged text uses Umicom C. Tags cannot nest. Regions exclude tag bytes;
 * ordinary C-style quoted strings and comments protect delimiters. BASIC
 * supports doubled quotes and apostrophe comments for boundary scanning only.
 * This is an experimental lexical envelope, not a C23/BASIC/Java parser:
 * Java Unicode preprocessing/text blocks and BASIC REM are not implemented.
 * Use separate source files for those forms. No region is executed or compiled.
 * Output count is zero on failure; partial region bytes must not then be used.
 * All regions must be disjoint, valid storage; output contains no borrowed pointer. */
UmcStatus umc_source_regions(const char *source,size_t length,UmcSourceRegion *regions,
    size_t capacity,size_t *count,UmcDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
