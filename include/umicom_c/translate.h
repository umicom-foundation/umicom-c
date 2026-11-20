/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/translate.h
 *
 * PURPOSE:
 *   Translate a checked integer function between small C and Java profiles.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMICOM_C_TRANSLATE_H
#define UMICOM_C_TRANSLATE_H

#include "umicom_c/umicom_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UMC_TRANSLATION_EXPRESSION_LIMIT 1024U
#define UMC_TRANSLATION_OUTPUT_CAPACITY (UMC_TRANSLATION_EXPRESSION_LIMIT + 128U)
#define UMC_TRANSLATION_FUNCTION_LIMIT 4096U
#define UMC_TRANSLATION_STATEMENT_LIMIT 32U
#define UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY (UMC_TRANSLATION_FUNCTION_LIMIT + 512U)
/* SDK consumers can admit comment workflows only when this profile is present. */
#define UMC_TRANSLATION_PRESERVES_BODY_COMMENTS 1

typedef enum UmcTranslationDirection {
    UMC_TRANSLATE_C_TO_JAVA = 1,
    UMC_TRANSLATE_JAVA_TO_C
} UmcTranslationDirection;

/* Translate one int answer(void) definition or one public final UmicomAnswer
 * class containing a public static int answer() method. The body is one
 * return expression with decimal int32 literals and checked arithmetic.
 * The C consumer must use a 32-bit int. This profile accepts ASCII only.
 *
 * All source, output, length and optional diagnostic storage is disjoint.
 * Successful output is NUL terminated and its reported length excludes
 * that terminator. Failures preserve the output and output length. */
UmcStatus umc_translate_integer_answer(UmcTranslationDirection direction, const char *source,
    size_t source_size, char *output, size_t output_capacity, size_t *output_size,
    UmcDiagnostic *diagnostic);

/* Extend the same function envelope to initialized int locals, assignments
 * with = and one final return. Local names must be plain identifiers from
 * the common C/Java subset. Each expression uses the original checked
 * arithmetic and may read previously initialized locals. The shared compiler
 * confirms the complete sequence executes within the original program bound.
 * An expression-only body is passed through the existing translator without
 * changing its original limits or output. Functions with locals use the
 * function and statement limits above. The ownership contract is unchanged. */
/* The function entry also preserves ordinary ASCII line and block comments
 * inside the body. Commented input uses the function-size limit. Comments
 * that contain Java Unicode escapes or C line splices are refused. */
UmcStatus umc_translate_integer_function(UmcTranslationDirection direction, const char *source,
    size_t source_size, char *output, size_t output_capacity, size_t *output_size,
    UmcDiagnostic *diagnostic);

#ifdef __cplusplus
}
#endif

#endif
