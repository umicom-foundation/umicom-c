/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/compiler.c
 *
 * PURPOSE:
 *   Compile a specified arithmetic grammar to checked stack instructions.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "internal.h"

typedef struct UmcParser {
    UmcLexer lexer;
    UmcToken token;
    UmcInstruction *output;
    size_t capacity;
    size_t count;
    UmcDiagnostic *diagnostic;
    UmcStatus status;
} UmcParser;

static void advance(UmcParser *parser)
{
    if (parser->status == UMC_OK) {
        parser->status = umc_lex_next(&parser->lexer, &parser->token, parser->diagnostic);
    }
}

static void expect(UmcParser *parser, UmcTokenKind kind)
{
    if (parser->status != UMC_OK) return;
    if (parser->token.kind != kind) {
        parser->status = umc_fail(parser->diagnostic, UMC_SYNTAX_ERROR, parser->token.offset);
        return;
    }
    advance(parser);
}

static void emit(UmcParser *parser, UmcOpcode opcode, int32_t operand, size_t offset)
{
    if (parser->status != UMC_OK) return;
    if (parser->count >= parser->capacity || parser->count >= UMC_PROGRAM_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_PROGRAM_TOO_LARGE, offset);
        return;
    }
    UmcInstruction *instruction = &parser->output[parser->count++];
    instruction->opcode = opcode;
    instruction->operand = operand;
    instruction->source_offset = offset;
}

static void expression(UmcParser *parser, size_t depth);

static void unary(UmcParser *parser, size_t depth)
{
    if (parser->status != UMC_OK) return;
    if (depth >= UMC_PARSE_DEPTH_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_DEPTH_LIMIT, parser->token.offset);
        return;
    }
    const UmcToken token = parser->token;
    if (token.kind == UMC_TOKEN_PLUS || token.kind == UMC_TOKEN_MINUS) {
        advance(parser);
        unary(parser, depth + 1U);
        if (token.kind == UMC_TOKEN_MINUS) emit(parser, UMC_NEGATE, 0, token.offset);
    } else if (token.kind == UMC_TOKEN_INTEGER) {
        emit(parser, UMC_PUSH, token.integer, token.offset);
        advance(parser);
    } else if (token.kind == UMC_TOKEN_LEFT) {
        advance(parser);
        expression(parser, depth + 1U);
        expect(parser, UMC_TOKEN_RIGHT);
    } else {
        parser->status = umc_fail(parser->diagnostic, UMC_SYNTAX_ERROR, token.offset);
    }
}

static void product(UmcParser *parser, size_t depth)
{
    unary(parser, depth);
    while (parser->status == UMC_OK &&
           (parser->token.kind == UMC_TOKEN_STAR || parser->token.kind == UMC_TOKEN_SLASH)) {
        const UmcToken token = parser->token;
        advance(parser);
        unary(parser, depth);
        emit(parser, token.kind == UMC_TOKEN_STAR ? UMC_MULTIPLY : UMC_DIVIDE, 0, token.offset);
    }
}

static void expression(UmcParser *parser, size_t depth)
{
    /* Separate precedence levels ensure 2 + 3 * 4 emits multiplication first.
     * Iteration makes repeated binary operators left associative without adding
     * a recursive call for every term in a long expression. */
    product(parser, depth);
    while (parser->status == UMC_OK &&
           (parser->token.kind == UMC_TOKEN_PLUS || parser->token.kind == UMC_TOKEN_MINUS)) {
        const UmcToken token = parser->token;
        advance(parser);
        product(parser, depth);
        emit(parser, token.kind == UMC_TOKEN_PLUS ? UMC_ADD : UMC_SUBTRACT, 0, token.offset);
    }
}

UmcStatus umc_compile(const char *source, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    if (count != NULL) *count = 0U;
    (void)umc_fail(diagnostic, UMC_OK, 0U);
    if (source == NULL || output == NULL || count == NULL || capacity == 0U) {
        return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    }
    if (length > UMC_SOURCE_LIMIT) {
        return umc_fail(diagnostic, UMC_SOURCE_TOO_LARGE, 0U);
    }
    UmcParser parser = {
        .lexer = {source, length, 0U},
        .output = output,
        .capacity = capacity,
        .diagnostic = diagnostic,
        .status = UMC_OK
    };
    advance(&parser);
    expect(&parser, UMC_TOKEN_INT);
    expect(&parser, UMC_TOKEN_MAIN);
    expect(&parser, UMC_TOKEN_LEFT);
    expect(&parser, UMC_TOKEN_VOID);
    expect(&parser, UMC_TOKEN_RIGHT);
    expect(&parser, UMC_TOKEN_OPEN);
    const size_t return_offset = parser.token.offset;
    expect(&parser, UMC_TOKEN_RETURN);
    expression(&parser, 0U);
    expect(&parser, UMC_TOKEN_SEMICOLON);
    expect(&parser, UMC_TOKEN_CLOSE);
    expect(&parser, UMC_TOKEN_END);
    emit(&parser, UMC_RETURN, 0, return_offset);
    if (parser.status == UMC_OK) {
        parser.status = umc_validate(output, parser.count, diagnostic);
    }
    if (parser.status == UMC_OK) {
        *count = parser.count;
    } else {
        umc_locate_diagnostic(source, length, diagnostic);
    }
    return parser.status;
}
