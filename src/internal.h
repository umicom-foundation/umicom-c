/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/internal.h
 *
 * PURPOSE:
 *   Keep lexical and diagnostic helpers private to the compiler implementation.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#ifndef UMC_INTERNAL_H
#define UMC_INTERNAL_H
#include "umicom_c/umicom_c.h"

typedef enum UmcTokenKind {
    UMC_TOKEN_END = 0, UMC_TOKEN_INT, UMC_TOKEN_MAIN, UMC_TOKEN_VOID,
    UMC_TOKEN_RETURN, UMC_TOKEN_INTEGER, UMC_TOKEN_PLUS, UMC_TOKEN_MINUS,
    UMC_TOKEN_STAR, UMC_TOKEN_SLASH, UMC_TOKEN_LEFT, UMC_TOKEN_RIGHT,
    UMC_TOKEN_OPEN, UMC_TOKEN_CLOSE, UMC_TOKEN_SEMICOLON
} UmcTokenKind;

typedef struct UmcToken {
    UmcTokenKind kind;
    int32_t integer;
    size_t offset;
} UmcToken;

typedef struct UmcLexer {
    const char *source;
    size_t length;
    size_t position;
} UmcLexer;

UmcStatus umc_fail(UmcDiagnostic *diagnostic, UmcStatus status, size_t offset);
UmcStatus umc_lex_next(UmcLexer *lexer, UmcToken *token, UmcDiagnostic *diagnostic);
#endif
