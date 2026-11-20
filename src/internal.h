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
    UMC_TOKEN_OPEN, UMC_TOKEN_CLOSE, UMC_TOKEN_SEMICOLON,
    UMC_TOKEN_LESS, UMC_TOKEN_LESS_EQUAL, UMC_TOKEN_GREATER, UMC_TOKEN_GREATER_EQUAL,
    UMC_TOKEN_EQUAL, UMC_TOKEN_NOT_EQUAL, UMC_TOKEN_NOT,
    UMC_TOKEN_IDENTIFIER, UMC_TOKEN_ASSIGN, UMC_TOKEN_AND, UMC_TOKEN_OR,
    UMC_TOKEN_QUESTION, UMC_TOKEN_COLON,
    UMC_TOKEN_ADD_ASSIGN, UMC_TOKEN_SUBTRACT_ASSIGN,
    UMC_TOKEN_MULTIPLY_ASSIGN, UMC_TOKEN_DIVIDE_ASSIGN,
    UMC_TOKEN_IF, UMC_TOKEN_ELSE,
    UMC_TOKEN_REMAINDER, UMC_TOKEN_REMAINDER_ASSIGN, UMC_TOKEN_COMPLEMENT,
    UMC_TOKEN_BITWISE_AND, UMC_TOKEN_BITWISE_XOR, UMC_TOKEN_BITWISE_OR,
    UMC_TOKEN_BITWISE_AND_ASSIGN, UMC_TOKEN_BITWISE_XOR_ASSIGN, UMC_TOKEN_BITWISE_OR_ASSIGN,
    UMC_TOKEN_INCREMENT, UMC_TOKEN_DECREMENT,
    UMC_TOKEN_CONST,
    UMC_TOKEN_SHIFT_LEFT, UMC_TOKEN_SHIFT_RIGHT,
    UMC_TOKEN_SHIFT_LEFT_ASSIGN, UMC_TOKEN_SHIFT_RIGHT_ASSIGN,
    UMC_TOKEN_COMMA /* Declaration separator only; no comma expression. */
} UmcTokenKind;

typedef struct UmcToken {
    UmcTokenKind kind;
    int32_t integer;
    size_t offset;
    size_t length; /* Borrowed source spelling; the lexer never allocates names. */
} UmcToken;

typedef struct UmcLexer {
    const char *source;
    size_t length;
    size_t position;
} UmcLexer;

UmcStatus umc_fail(UmcDiagnostic *diagnostic, UmcStatus status, size_t offset);
/* The compiler and bounded source translator share the same comment extent. */
bool umc_lex_at_comment(const UmcLexer *lexer);
UmcStatus umc_lex_skip_comment(UmcLexer *lexer, UmcDiagnostic *diagnostic);
UmcStatus umc_lex_next(UmcLexer *lexer, UmcToken *token, UmcDiagnostic *diagnostic);
#endif
