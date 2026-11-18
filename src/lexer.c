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


/* A separator is spelling, not an arithmetic operation. Validate the following
 * digit in the current radix before skipping the quote, keeping both overflow
 * checks and source-span admission in the existing integer accumulator. */
static bool separator_digit(unsigned char c, int32_t base)
{
    if (digit(c)) return (int32_t)(c - (unsigned char)'0') < base;
    return base == 16 && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
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

        if (c == '\'') {
            if (lexer->position == digits_start ||
                lexer->length - lexer->position < 2U ||
                !separator_digit((unsigned char)lexer->source[lexer->position + 1U], base))
                return umc_fail(diagnostic, UMC_SYNTAX_ERROR, lexer->position);
            ++lexer->position;
            continue;
        }

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


/* Character spelling is decoded once by the lexer into an ordinary integer.
 * This explicit ASCII profile avoids host char signedness, locale and execution
 * encoding assumptions. All lookahead remains inside the borrowed source span. */
static UmcStatus ascii_character(UmcLexer *lexer, UmcToken *token,
    UmcDiagnostic *diagnostic, size_t start)
{
    if (lexer->position >= lexer->length)
        return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
    unsigned char c = (unsigned char)lexer->source[lexer->position++];
    int32_t value = 0;
    if (c == '\\') {
        if (lexer->position >= lexer->length)
            return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
        c = (unsigned char)lexer->source[lexer->position++];
        switch (c) {
        case 'a': value = 7; break;
        case 'b': value = 8; break;
        case 'f': value = 12; break;
        case 'n': value = 10; break;
        case 'r': value = 13; break;
        case 't': value = 9; break;
        case 'v': value = 11; break;
        case '\\': value = 92; break;
        case '\'': value = 39; break;
        case '"': value = 34; break;
        case '?': value = 63; break;
        case 'x': {
            const size_t firstDigit = lexer->position;
            while (lexer->position < lexer->length &&
                separator_digit((unsigned char)lexer->source[lexer->position], 16)) {
                const unsigned char part = (unsigned char)lexer->source[lexer->position++];
                const int32_t digitValue = digit(part) ? (int32_t)(part - '0') :
                    part >= 'a' ? (int32_t)(part - 'a') + 10 : (int32_t)(part - 'A') + 10;
                if (value > (127 - digitValue) / 16)
                    return umc_fail(diagnostic, UMC_LITERAL_RANGE, start);
                value = value * 16 + digitValue;
            }
            if (lexer->position == firstDigit)
                return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
            break;
        }
        default:
            if (c < '0' || c > '7')
                return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
            value = (int32_t)(c - '0');
            for (unsigned digits = 1U; digits < 3U && lexer->position < lexer->length; ++digits) {
                const unsigned char part = (unsigned char)lexer->source[lexer->position];
                if (part < '0' || part > '7') break;
                value = value * 8 + (int32_t)(part - '0');
                ++lexer->position;
            }
            if (value > 127) return umc_fail(diagnostic, UMC_LITERAL_RANGE, start);
            break;
        }
    } else {
        if (c < 32U || c > 126U || c == '\'')
            return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
        value = (int32_t)c;
    }
    /* Exactly one decoded value is allowed. A second character, a digit after
     * a three-digit octal escape or a missing quote is never silently ignored. */
    if (lexer->position >= lexer->length || lexer->source[lexer->position] != '\'')
        return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
    ++lexer->position;
    token->kind = UMC_TOKEN_INTEGER;
    token->integer = value;
    token->length = lexer->position - start;
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
    if (c == '\'') return ascii_character(lexer, token, diagnostic, start);
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
        /* Retain the digit-only loop for grammar history. The active loop
         * below admits quotes only between digits and preserves its checked
         * decimal value, overflow location and borrowed-source boundary. */
#if 0
        while (lexer->position < lexer->length &&
               digit((unsigned char)lexer->source[lexer->position])) {
            const int32_t part = (int32_t)(lexer->source[lexer->position] - '0');
            if (value > (INT32_MAX - part) / 10) {
                return umc_fail(diagnostic, UMC_LITERAL_RANGE, start);
            }
            value = value * 10 + part;
            ++lexer->position;
        }
#endif
        while (lexer->position < lexer->length) {
            const unsigned char next = (unsigned char)lexer->source[lexer->position];
            if (next == '\'') {
                if (lexer->length - lexer->position < 2U ||
                    !separator_digit((unsigned char)lexer->source[lexer->position + 1U], 10))
                    return umc_fail(diagnostic, UMC_SYNTAX_ERROR, lexer->position);
                ++lexer->position;
                continue;
            }
            if (!digit(next)) break;
            const int32_t part = (int32_t)(next - (unsigned char)'0');
            if (value > (INT32_MAX - part) / 10)
                return umc_fail(diagnostic, UMC_LITERAL_RANGE, start);
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
/* Recognize const only as a complete keyword; longer identifier spellings retain their existing meaning. The former implementation is retained for owner review. */
#if 0
        if (matches(word, length, "int")) token->kind = UMC_TOKEN_INT;
#endif
        if (matches(word, length, "const")) token->kind = UMC_TOKEN_CONST;
        else if (matches(word, length, "int")) token->kind = UMC_TOKEN_INT;
        else if (matches(word, length, "main")) token->kind = UMC_TOKEN_MAIN;
        else if (matches(word, length, "void")) token->kind = UMC_TOKEN_VOID;
        else if (matches(word, length, "return")) token->kind = UMC_TOKEN_RETURN;
        /* Reserve complete control-flow words; prefixes such as if_value
         * remain ordinary identifiers in the existing bounded name grammar. */
        else if (matches(word, length, "if")) token->kind = UMC_TOKEN_IF;
        else if (matches(word, length, "else")) token->kind = UMC_TOKEN_ELSE;
        /* Literal spelling belongs to the lexer. Lower complete, case-sensitive
         * keywords to ordinary integer tokens so execution, source warnings
         * and portable bytecode keep their existing owners. Longer names are
         * still identifiers; this does not introduce a typed bool local. */
        else if (matches(word, length, "true") || matches(word, length, "false")) {
            token->kind = UMC_TOKEN_INTEGER;
            token->integer = matches(word, length, "true") ? 1 : 0;
        }
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
    /* Arithmetic characters now admit an adjacent assignment suffix. Preserve
     * the single-character predecessor for review; comments and whitespace
     * cannot join two tokens into a compound assignment. Lookahead never reads
     * beyond the borrowed source span, including a final arithmetic character. */
#if 0
    case '+': token->kind = UMC_TOKEN_PLUS; break;
    case '-': token->kind = UMC_TOKEN_MINUS; break;
    case '*': token->kind = UMC_TOKEN_STAR; break;
    case '/': token->kind = UMC_TOKEN_SLASH; break;
#endif
/* Adjacent increment/decrement now have their own bounded tokens; existing arithmetic and compound-assignment behavior is retained. The former implementation is retained for owner review. */
#if 0
    case '+': case '-': case '*': case '/': {
        const bool paired = lexer->position < lexer->length &&
            lexer->source[lexer->position] == '=';
        if (paired) {
            ++lexer->position;
            token->length = 2U;
        }
        if (c == '+') token->kind = paired ? UMC_TOKEN_ADD_ASSIGN : UMC_TOKEN_PLUS;
        else if (c == '-') token->kind = paired ? UMC_TOKEN_SUBTRACT_ASSIGN : UMC_TOKEN_MINUS;
        else if (c == '*') token->kind = paired ? UMC_TOKEN_MULTIPLY_ASSIGN : UMC_TOKEN_STAR;
        else token->kind = paired ? UMC_TOKEN_DIVIDE_ASSIGN : UMC_TOKEN_SLASH;
        break;
    }

#endif
    case '+': case '-': case '*': case '/': {
        /* Longest match keeps ++/-- separate from two spaced unary signs.
         * Only initialized-local statement parsing consumes these tokens. */
        if ((c == '+' || c == '-') && lexer->position < lexer->length &&
            lexer->source[lexer->position] == (char)c) {
            ++lexer->position;
            token->length = 2U;
            token->kind = c == '+' ? UMC_TOKEN_INCREMENT : UMC_TOKEN_DECREMENT;
            break;
        }
        const bool paired = lexer->position < lexer->length &&
            lexer->source[lexer->position] == '=';
        if (paired) {
            ++lexer->position;
            token->length = 2U;
        }
        if (c == '+') token->kind = paired ? UMC_TOKEN_ADD_ASSIGN : UMC_TOKEN_PLUS;
        else if (c == '-') token->kind = paired ? UMC_TOKEN_SUBTRACT_ASSIGN : UMC_TOKEN_MINUS;
        else if (c == '*') token->kind = paired ? UMC_TOKEN_MULTIPLY_ASSIGN : UMC_TOKEN_STAR;
        else token->kind = paired ? UMC_TOKEN_DIVIDE_ASSIGN : UMC_TOKEN_SLASH;
        break;
    }
    case '(': token->kind = UMC_TOKEN_LEFT; break;
    case ')': token->kind = UMC_TOKEN_RIGHT; break;
    case '{': token->kind = UMC_TOKEN_OPEN; break;
    case '}': token->kind = UMC_TOKEN_CLOSE; break;
    case ';': token->kind = UMC_TOKEN_SEMICOLON; break;
    /* Parsing decides whether this separator belongs to a declaration list. */
    case ',': token->kind = UMC_TOKEN_COMMA; break;
    /* Remainder uses bounded adjacent lookahead; comments and whitespace
     * cannot combine % and = into an assignment token. */
    case '%':
        if (lexer->position < lexer->length && lexer->source[lexer->position] == '=') {
            ++lexer->position;
            token->length = 2U;
            token->kind = UMC_TOKEN_REMAINDER_ASSIGN;
        } else token->kind = UMC_TOKEN_REMAINDER;
        break;
    /* A standalone unary token needs no lookahead or runtime allocation. */
    case '~': token->kind = UMC_TOKEN_COMPLEMENT; break;
    case '?': token->kind = UMC_TOKEN_QUESTION; break;
    case ':': token->kind = UMC_TOKEN_COLON; break;
    /* Preserve the logical-only token branch. The replacement keeps longest
     * match for &&/|| and admits separate eager operators and assignments. */
#if 0
    case '&': case '|':
        /* Single characters remain unsupported bitwise operators. Lookahead is
         * bounded by the borrowed span and cannot join tokens across whitespace. */
        if (lexer->position == lexer->length || lexer->source[lexer->position] != (char)c)
            return umc_fail(diagnostic, UMC_SYNTAX_ERROR, start);
        ++lexer->position;
        token->length = 2U;
        token->kind = c == '&' ? UMC_TOKEN_AND : UMC_TOKEN_OR;
        break;
#endif
    case '&': case '|': case '^': {
        const char next = lexer->position < lexer->length ?
            lexer->source[lexer->position] : '\0';
        if (c != '^' && next == (char)c) {
            ++lexer->position;
            token->length = 2U;
            token->kind = c == '&' ? UMC_TOKEN_AND : UMC_TOKEN_OR;
        } else {
            const bool assignment = next == '=';
            if (assignment) {
                ++lexer->position;
                token->length = 2U;
            }
            token->kind = c == '&' ?
                (assignment ? UMC_TOKEN_BITWISE_AND_ASSIGN : UMC_TOKEN_BITWISE_AND) :
                c == '^' ?
                (assignment ? UMC_TOKEN_BITWISE_XOR_ASSIGN : UMC_TOKEN_BITWISE_XOR) :
                (assignment ? UMC_TOKEN_BITWISE_OR_ASSIGN : UMC_TOKEN_BITWISE_OR);
        }
        break;
    }
    /* Longest-match lookahead stays within the borrowed source span. Unary
     * logical negation is distinct from !=; assignment remains unsupported. */
    case '<': case '>': case '=': case '!': {
        /* Consume the longest shift token within the borrowed byte span.
         * A spaced or commented pair cannot become one shift operator. */
        if ((c == '<' || c == '>') && lexer->position < lexer->length &&
            lexer->source[lexer->position] == (char)c) {
            ++lexer->position;
            token->length = 2U;
            const bool assignment = lexer->position < lexer->length &&
                lexer->source[lexer->position] == '=';
            if (assignment) { ++lexer->position; token->length = 3U; }
            token->kind = c == '<' ?
                (assignment ? UMC_TOKEN_SHIFT_LEFT_ASSIGN : UMC_TOKEN_SHIFT_LEFT) :
                (assignment ? UMC_TOKEN_SHIFT_RIGHT_ASSIGN : UMC_TOKEN_SHIFT_RIGHT);
            break;
        }
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
