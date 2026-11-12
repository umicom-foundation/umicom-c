/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/lexer.c
 *
 * PURPOSE:
 *   Read bounded source bytes into tokens without assuming a terminating NUL.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "internal.h"
#include <limits.h>
#include <stdbool.h>

static bool identifier_start(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool digit(unsigned char c)
{
    return c >= '0' && c <= '9';
}

static bool matches(const char *source, size_t length, const char *word)
{
    size_t i = 0U;
    while (i < length && word[i] != '\0' && source[i] == word[i]) {
        ++i;
    }
    return i == length && word[i] == '\0';
}


/* Numeric spelling is a compiler concern. Decode a bounded ASCII token into
 * the same signed PUSH value used by decimal input; the VM and wire format do
 * not acquire radix-specific operations. Never rely on a trailing NUL, locale,
 * strtol, unsigned wrap, or a host-sized long. The caller consumed the zero. */
static UmcStatus prefixed_integer(UmcLexer *lexer, UmcToken *token,
    UmcDiagnostic *diagnostic, size_t start, int32_t base)
{
    ++lexer->position; /* Consume the admitted x/X/b/B prefix byte. */
    const size_t digits_start = lexer->position;
    int32_t value = 0;
    while (lexer->position < lexer->length) {
        const unsigned char c = (unsigned char)lexer->source[lexer->position];
        int32_t part;
        if (digit(c)) part = (int32_t)(c - (unsigned char)'0');
        else if (c >= 'a' && c <= 'f') part = (int32_t)(c - (unsigned char)'a') + 10;
        else if (c >= 'A' && c <= 'F') part = (int32_t)(c - (unsigned char)'A') + 10;
        else break;
        if (part >= base)
            return umc_fail(diagnostic, UMC_SYNTAX_ERROR, lexer->position);
        /* Check before multiplication so even the rejected token never causes
         * signed overflow. Range diagnostics identify the start of the token. */
        if (value > (INT32_MAX - part) / base)
            return umc_fail(diagnostic, UMC_LITERAL_RANGE, start);
        value = value * base + part;
        ++lexer->position;
    }
    if (lexer->position == digits_start)
        return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
    if (lexer->position < lexer->length &&
        identifier_start((unsigned char)lexer->source[lexer->position]))
        return umc_fail(diagnostic, UMC_SYNTAX_ERROR, lexer->position);
    token->kind = UMC_TOKEN_INTEGER;
    token->integer = value;
    return UMC_OK;
}

UmcStatus umc_lex_next(UmcLexer *lexer, UmcToken *token, UmcDiagnostic *diagnostic)
{
    /* Source length was admitted by umc_compile. Lookahead always checks the
     * remaining byte count; a comment never authorizes reading past that span. */
    for (;;) {
        if (lexer->position == lexer->length) {
            token->length = 0U;
            token->kind = UMC_TOKEN_END;
            token->integer = 0;
            token->offset = lexer->position;
            return UMC_OK;
        }
        const unsigned char c = (unsigned char)lexer->source[lexer->position];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v') {
            ++lexer->position;
            continue;
        }
        if (c == '/' && lexer->length - lexer->position >= 2U) {
            const char next = lexer->source[lexer->position + 1U];
            if (next == '/') {
                lexer->position += 2U;
                while (lexer->position < lexer->length &&
                       lexer->source[lexer->position] != '\n' &&
                       lexer->source[lexer->position] != '\r') {
                    if (lexer->source[lexer->position] == '\0') {
                        return umc_fail(diagnostic, UMC_SYNTAX_ERROR, lexer->position);
                    }
                    ++lexer->position;
                }
                continue;
            }
            if (next == '*') {
                const size_t start = lexer->position;
                lexer->position += 2U;
                bool closed = false;
                while (lexer->position < lexer->length) {
                    if (lexer->source[lexer->position] == '\0') {
                        return umc_fail(diagnostic, UMC_SYNTAX_ERROR, lexer->position);
                    }
                    if (lexer->length - lexer->position >= 2U &&
                        lexer->source[lexer->position] == '*' &&
                        lexer->source[lexer->position + 1U] == '/') {
                        lexer->position += 2U;
                        closed = true;
                        break;
                    }
                    ++lexer->position;
                }
                if (!closed) {
                    return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
                }
                continue;
            }
        }
        break;
    }

    const size_t start = lexer->position;
    const unsigned char c = (unsigned char)lexer->source[lexer->position++];
    token->offset = start;
    token->length = 1U;
    token->integer = 0;
    /* Explicit prefixes extend the grammar; unprefixed leading-zero tokens
     * continue through the original decimal implementation below. */
    if (c == '0' && lexer->position < lexer->length) {
        const char prefix = lexer->source[lexer->position];
        if (prefix == 'x' || prefix == 'X')
            return prefixed_integer(lexer, token, diagnostic, start, 16);
        if (prefix == 'b' || prefix == 'B')
            return prefixed_integer(lexer, token, diagnostic, start, 2);
    }
    if (digit(c)) {
        int32_t value = (int32_t)(c - (unsigned char)'0');
        while (lexer->position < lexer->length &&
               digit((unsigned char)lexer->source[lexer->position])) {
            const int32_t part = (int32_t)(lexer->source[lexer->position] - '0');
            if (value > (INT32_MAX - part) / 10) {
                return umc_fail(diagnostic, UMC_LITERAL_RANGE, start);
            }
            value = value * 10 + part;
            ++lexer->position;
        }
        token->kind = UMC_TOKEN_INTEGER;
        token->integer = value;
        return UMC_OK;
    }
    if (identifier_start(c)) {
        while (lexer->position < lexer->length) {
            const unsigned char next = (unsigned char)lexer->source[lexer->position];
            if (!identifier_start(next) && !digit(next)) {
                break;
            }
            ++lexer->position;
        }
        const size_t length = lexer->position - start;
        const char *word = lexer->source + start;
        if (matches(word, length, "int")) token->kind = UMC_TOKEN_INT;
        else if (matches(word, length, "main")) token->kind = UMC_TOKEN_MAIN;
        else if (matches(word, length, "void")) token->kind = UMC_TOKEN_VOID;
        else if (matches(word, length, "return")) token->kind = UMC_TOKEN_RETURN;
        /* Non-keywords now form bounded local identifiers. Keep the former
         * keyword-only refusal for review; the parser still controls every use. */
#if 0
        else return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
#endif
        else {
            if (length > UMC_IDENTIFIER_LIMIT)
                return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
            token->kind = UMC_TOKEN_IDENTIFIER;
        }
        token->length = length;
        return UMC_OK;
    }
    switch (c) {
    case '+': token->kind = UMC_TOKEN_PLUS; break;
    case '-': token->kind = UMC_TOKEN_MINUS; break;
    case '*': token->kind = UMC_TOKEN_STAR; break;
    case '/': token->kind = UMC_TOKEN_SLASH; break;
    case '(': token->kind = UMC_TOKEN_LEFT; break;
    case ')': token->kind = UMC_TOKEN_RIGHT; break;
    case '{': token->kind = UMC_TOKEN_OPEN; break;
    case '}': token->kind = UMC_TOKEN_CLOSE; break;
    case ';': token->kind = UMC_TOKEN_SEMICOLON; break;
    case '?': token->kind = UMC_TOKEN_QUESTION; break;
    case ':': token->kind = UMC_TOKEN_COLON; break;
    case '&': case '|':
        /* Single characters remain unsupported bitwise operators. Lookahead is
         * bounded by the borrowed span and cannot join tokens across whitespace. */
        if (lexer->position == lexer->length || lexer->source[lexer->position] != (char)c)
            return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
        ++lexer->position;
        token->length = 2U;
        token->kind = c == '&' ? UMC_TOKEN_AND : UMC_TOKEN_OR;
        break;
    /* Longest-match lookahead stays within the borrowed source span. Unary
     * logical negation is distinct from !=; assignment remains unsupported. */
    case '<': case '>': case '=': case '!': {
        const bool paired = lexer->position < lexer->length && lexer->source[lexer->position] == '=';
        /* The earlier profile rejected both unpaired characters. Retain its
         * admission rule for review; the new branch admits only unary !. */
#if 0
    /* Longest-match lookahead stays within the borrowed source span. A bare
     * '=' or '!' is outside this expression profile and fails at its own byte. */
        if ((c == '=' || c == '!') && !paired)
            return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
#endif
        /* Assignment is admitted as a statement token, not an expression.
         * Equality longest-match remains unchanged. */
#if 0
        if (c == '=' && !paired)
            return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
#endif
        if (c == '=' && !paired) {
            token->kind = UMC_TOKEN_ASSIGN;
            break;
        }
        if (c == '!' && !paired) {
            token->kind = UMC_TOKEN_NOT;
            break;
        }
        if (paired) ++lexer->position;
        if (c == '<') token->kind = paired ? UMC_TOKEN_LESS_EQUAL : UMC_TOKEN_LESS;
        else if (c == '>') token->kind = paired ? UMC_TOKEN_GREATER_EQUAL : UMC_TOKEN_GREATER;
        else token->kind = c == '=' ? UMC_TOKEN_EQUAL : UMC_TOKEN_NOT_EQUAL;
        break;
    }
    default: return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
    }
    return UMC_OK;
}
