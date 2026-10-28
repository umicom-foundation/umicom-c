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

UmcStatus umc_lex_next(UmcLexer *lexer, UmcToken *token, UmcDiagnostic *diagnostic)
{
    /* Source length was admitted by umc_compile. Lookahead always checks the
     * remaining byte count; a comment never authorizes reading past that span. */
    for (;;) {
        if (lexer->position == lexer->length) {
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
    token->integer = 0;
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
        else return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
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
    default: return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
    }
    return UMC_OK;
}
