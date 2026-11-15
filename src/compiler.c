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
#include "umicom_c/profile.h"

typedef struct UmcParser {
    UmcLexer lexer;
    UmcToken token;
    UmcInstruction *output;
    size_t capacity;
    size_t count;
    UmcDiagnostic *diagnostic;
    UmcStatus status;
    struct { size_t offset; size_t length; } locals[UMC_LOCAL_LIMIT];
    size_t local_count;
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

/* Names borrow the admitted source span for this compile call. Slot numbers
 * follow declaration order; no source pointer enters the bytecode or VM. */
static size_t find_local(const UmcParser *parser, const UmcToken *name)
{
    for (size_t i = 0U; i < parser->local_count; ++i) {
        if (parser->locals[i].length != name->length) continue;
        size_t matched = 0U;
        while (matched < name->length &&
            parser->lexer.source[parser->locals[i].offset + matched] ==
            parser->lexer.source[name->offset + matched]) ++matched;
        if (matched == name->length) return i;
    }
    return UMC_LOCAL_LIMIT;
}
static void local_error(UmcParser *parser, size_t offset, const char *message)
{
    if (parser->status != UMC_OK) return;
    parser->status = umc_fail(parser->diagnostic, UMC_SYNTAX_ERROR, offset);
    if (parser->diagnostic != NULL) parser->diagnostic->message = message;
}


/* The arithmetic-only unary parser remains available for source review.
 * Its replacement adds ! by composing existing validated VM operations;
 * arithmetic precedence, depth bounds and failure publication are retained. */
#if 0
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

#endif

/* Resolve identifiers to validated local slots while preserving unary precedence and bounded recursion. The predecessor remains disabled for review. */
#if 0
static void unary(UmcParser *parser, size_t depth)
{
    if (parser->status != UMC_OK) return;
    if (depth >= UMC_PARSE_DEPTH_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_DEPTH_LIMIT, parser->token.offset);
        return;
    }
    const UmcToken token = parser->token;
    if (token.kind == UMC_TOKEN_PLUS || token.kind == UMC_TOKEN_MINUS ||
        token.kind == UMC_TOKEN_NOT) {
        advance(parser);
        unary(parser, depth + 1U);
        if (token.kind == UMC_TOKEN_MINUS) emit(parser, UMC_NEGATE, 0, token.offset);
        if (token.kind == UMC_TOKEN_NOT) {
            /* Lower to the shared comparison runtime. One extra constant and
             * equality produce 0 or 1 without a new wire opcode or host path.
             * Both operations retain the ! byte for fuel/trace diagnostics. */
            emit(parser, UMC_PUSH, 0, token.offset);
            emit(parser, UMC_EQUAL, 0, token.offset);
        }
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

#endif

/* Preserve the earlier unary parser for review. Complement now lowers through checked subtraction while retaining precedence, recursion bounds and operand diagnostics. The previous implementation is retained for engineering review. */
#if 0
static void unary(UmcParser *parser, size_t depth)
{
    if (parser->status != UMC_OK) return;
    if (depth >= UMC_PARSE_DEPTH_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_DEPTH_LIMIT, parser->token.offset);
        return;
    }
    const UmcToken token = parser->token;
    if (token.kind == UMC_TOKEN_PLUS || token.kind == UMC_TOKEN_MINUS ||
        token.kind == UMC_TOKEN_NOT) {
        advance(parser);
        unary(parser, depth + 1U);
        if (token.kind == UMC_TOKEN_MINUS) emit(parser, UMC_NEGATE, 0, token.offset);
        if (token.kind == UMC_TOKEN_NOT) {
            /* Lower to the shared comparison runtime. One extra constant and
             * equality produce 0 or 1 without a new wire opcode or host path.
             * Both operations retain the ! byte for fuel/trace diagnostics. */
            emit(parser, UMC_PUSH, 0, token.offset);
            emit(parser, UMC_EQUAL, 0, token.offset);
        }
    } else if (token.kind == UMC_TOKEN_INTEGER) {
        emit(parser, UMC_PUSH, token.integer, token.offset);
        advance(parser);
    } else if (token.kind == UMC_TOKEN_IDENTIFIER) {
        const size_t slot = find_local(parser, &token);
        if (slot == UMC_LOCAL_LIMIT) {
            local_error(parser, token.offset, "local must be declared and initialized before use");
            return;
        }
        emit(parser, UMC_LOAD_LOCAL, (int32_t)slot, token.offset);
        advance(parser);
    } else if (token.kind == UMC_TOKEN_LEFT) {
        advance(parser);
        expression(parser, depth + 1U);
        expect(parser, UMC_TOKEN_RIGHT);
    } else {
        parser->status = umc_fail(parser->diagnostic, UMC_SYNTAX_ERROR, token.offset);
    }
}
#endif
static void unary(UmcParser *parser, size_t depth)
{
    if (parser->status != UMC_OK) return;
    if (depth >= UMC_PARSE_DEPTH_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_DEPTH_LIMIT, parser->token.offset);
        return;
    }
    const UmcToken token = parser->token;
    if (token.kind == UMC_TOKEN_PLUS || token.kind == UMC_TOKEN_MINUS ||
        token.kind == UMC_TOKEN_NOT || token.kind == UMC_TOKEN_COMPLEMENT) {
        /* For every signed 32-bit x, -1-x stays in range and equals its
         * bitwise complement. Push before evaluating x once; negating x first
         * would incorrectly overflow for INT32_MIN. No new runtime opcode. */
        if (token.kind == UMC_TOKEN_COMPLEMENT) emit(parser, UMC_PUSH, -1, token.offset);
        advance(parser);
        unary(parser, depth + 1U);
        if (token.kind == UMC_TOKEN_COMPLEMENT) emit(parser, UMC_SUBTRACT, 0, token.offset);
        if (token.kind == UMC_TOKEN_MINUS) emit(parser, UMC_NEGATE, 0, token.offset);
        if (token.kind == UMC_TOKEN_NOT) {
            /* Lower to the shared comparison runtime. One extra constant and
             * equality produce 0 or 1 without a new wire opcode or host path.
             * Both operations retain the ! byte for fuel/trace diagnostics. */
            emit(parser, UMC_PUSH, 0, token.offset);
            emit(parser, UMC_EQUAL, 0, token.offset);
        }
    } else if (token.kind == UMC_TOKEN_INTEGER) {
        emit(parser, UMC_PUSH, token.integer, token.offset);
        advance(parser);
    } else if (token.kind == UMC_TOKEN_IDENTIFIER) {
        const size_t slot = find_local(parser, &token);
        if (slot == UMC_LOCAL_LIMIT) {
            local_error(parser, token.offset, "local must be declared and initialized before use");
            return;
        }
        emit(parser, UMC_LOAD_LOCAL, (int32_t)slot, token.offset);
        advance(parser);
    } else if (token.kind == UMC_TOKEN_LEFT) {
        advance(parser);
        expression(parser, depth + 1U);
        expect(parser, UMC_TOKEN_RIGHT);
    } else {
        parser->status = umc_fail(parser->diagnostic, UMC_SYNTAX_ERROR, token.offset);
    }
}

/* Remainder shares multiplicative precedence and left associativity with multiplication and division. The previous implementation is retained for engineering review. */
#if 0
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
#endif
static void product(UmcParser *parser, size_t depth)
{
    unary(parser, depth);
    while (parser->status == UMC_OK &&
           (parser->token.kind == UMC_TOKEN_STAR || parser->token.kind == UMC_TOKEN_SLASH ||
            parser->token.kind == UMC_TOKEN_REMAINDER)) {
        const UmcToken token = parser->token;
        advance(parser);
        unary(parser, depth);
        const UmcOpcode opcode = token.kind == UMC_TOKEN_STAR ? UMC_MULTIPLY :
            token.kind == UMC_TOKEN_SLASH ? UMC_DIVIDE : UMC_REMAINDER;
        emit(parser, opcode, 0, token.offset);
    }
}

/* Retain arithmetic precedence in additive and place relational/equality parsing above it. The former implementation is retained for review. */
#if 0
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
#endif

static void additive(UmcParser *parser, size_t depth)
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

/* Relational and equality levels preserve C-style precedence. Each compares
 * two evaluated int32 values and emits a canonical zero-or-one stack result. */
static void relational(UmcParser *parser, size_t depth)
{
    additive(parser, depth);
    while (parser->status == UMC_OK &&
        (parser->token.kind == UMC_TOKEN_LESS || parser->token.kind == UMC_TOKEN_LESS_EQUAL ||
         parser->token.kind == UMC_TOKEN_GREATER || parser->token.kind == UMC_TOKEN_GREATER_EQUAL)) {
        const UmcToken token = parser->token;
        advance(parser);
        additive(parser, depth);
        UmcOpcode opcode = token.kind == UMC_TOKEN_LESS ? UMC_LESS :
            token.kind == UMC_TOKEN_LESS_EQUAL ? UMC_LESS_EQUAL :
            token.kind == UMC_TOKEN_GREATER ? UMC_GREATER : UMC_GREATER_EQUAL;
        emit(parser, opcode, 0, token.offset);
    }
}
/* Retain equality precedence and lower logical operators into explicit conditional paths so skipped operands never execute. The predecessor remains disabled for review. */
#if 0
static void expression(UmcParser *parser, size_t depth)
{
    relational(parser, depth);
    while (parser->status == UMC_OK &&
        (parser->token.kind == UMC_TOKEN_EQUAL || parser->token.kind == UMC_TOKEN_NOT_EQUAL)) {
        const UmcToken token = parser->token;
        advance(parser);
        relational(parser, depth);
        emit(parser, token.kind == UMC_TOKEN_EQUAL ? UMC_EQUAL : UMC_NOT_EQUAL, 0, token.offset);
    }
}

#endif

static void equality(UmcParser *parser, size_t depth)
{
    relational(parser, depth);
    while (parser->status == UMC_OK &&
        (parser->token.kind == UMC_TOKEN_EQUAL || parser->token.kind == UMC_TOKEN_NOT_EQUAL)) {
        const UmcToken token = parser->token;
        advance(parser);
        relational(parser, depth);
        emit(parser, token.kind == UMC_TOKEN_EQUAL ? UMC_EQUAL : UMC_NOT_EQUAL, 0, token.offset);
    }
}

/* Patching uses instruction indices only after their complete alternatives have
 * been emitted. Failure leaves count unpublished; partial output is never run. */
static void patch_forward(UmcParser *parser, size_t instruction)
{
    if (parser->status == UMC_OK)
        parser->output[instruction].operand = (int32_t)parser->count;
}
/* Separate eager precedence levels preserve C grouping without changing
 * logical short-circuit branches. These loops add no recursive parse depth. */
static void bitwise_and(UmcParser *parser, size_t depth)
{
    equality(parser, depth);
    while (parser->status == UMC_OK && parser->token.kind == UMC_TOKEN_BITWISE_AND) {
        const size_t offset = parser->token.offset;
        advance(parser);
        equality(parser, depth);
        emit(parser, UMC_BITWISE_AND, 0, offset);
    }
}
static void bitwise_xor(UmcParser *parser, size_t depth)
{
    bitwise_and(parser, depth);
    while (parser->status == UMC_OK && parser->token.kind == UMC_TOKEN_BITWISE_XOR) {
        const size_t offset = parser->token.offset;
        advance(parser);
        bitwise_and(parser, depth);
        emit(parser, UMC_BITWISE_XOR, 0, offset);
    }
}
static void bitwise_or(UmcParser *parser, size_t depth)
{
    bitwise_xor(parser, depth);
    while (parser->status == UMC_OK && parser->token.kind == UMC_TOKEN_BITWISE_OR) {
        const size_t offset = parser->token.offset;
        advance(parser);
        bitwise_xor(parser, depth);
        emit(parser, UMC_BITWISE_OR, 0, offset);
    }
}

/* Route logical operands through the new eager bitwise precedence levels while retaining the same forward short-circuit path. The previous implementation is retained for engineering review. */
#if 0
static void logical_and(UmcParser *parser, size_t depth)
{
    equality(parser, depth);
    while (parser->status == UMC_OK && parser->token.kind == UMC_TOKEN_AND) {
        const size_t offset = parser->token.offset;
        advance(parser);
        const size_t false_branch = parser->count;
        emit(parser, UMC_JUMP_IF_ZERO, 0, offset);
        equality(parser, depth);
        emit(parser, UMC_PUSH, 0, offset);
        emit(parser, UMC_NOT_EQUAL, 0, offset);
        const size_t join = parser->count;
        emit(parser, UMC_JUMP, 0, offset);
        patch_forward(parser, false_branch);
        emit(parser, UMC_PUSH, 0, offset);
        patch_forward(parser, join);
    }
}
#endif
static void logical_and(UmcParser *parser, size_t depth)
{
    bitwise_or(parser, depth);
    while (parser->status == UMC_OK && parser->token.kind == UMC_TOKEN_AND) {
        const size_t offset = parser->token.offset;
        advance(parser);
        const size_t false_branch = parser->count;
        emit(parser, UMC_JUMP_IF_ZERO, 0, offset);
        bitwise_or(parser, depth);
        emit(parser, UMC_PUSH, 0, offset);
        emit(parser, UMC_NOT_EQUAL, 0, offset);
        const size_t join = parser->count;
        emit(parser, UMC_JUMP, 0, offset);
        patch_forward(parser, false_branch);
        emit(parser, UMC_PUSH, 0, offset);
        patch_forward(parser, join);
    }
}
/* Keep logical OR precedence and add value-selecting conditional expressions through existing forward branch admission. The predecessor remains disabled for review. */
#if 0
static void expression(UmcParser *parser, size_t depth)
{
    logical_and(parser, depth);
    while (parser->status == UMC_OK && parser->token.kind == UMC_TOKEN_OR) {
        const size_t offset = parser->token.offset;
        advance(parser);
        const size_t right_branch = parser->count;
        emit(parser, UMC_JUMP_IF_ZERO, 0, offset);
        emit(parser, UMC_PUSH, 1, offset);
        const size_t join = parser->count;
        emit(parser, UMC_JUMP, 0, offset);
        patch_forward(parser, right_branch);
        logical_and(parser, depth);
        emit(parser, UMC_PUSH, 0, offset);
        emit(parser, UMC_NOT_EQUAL, 0, offset);
        patch_forward(parser, join);
    }
}

#endif

static void logical_or(UmcParser *parser, size_t depth)
{
    logical_and(parser, depth);
    while (parser->status == UMC_OK && parser->token.kind == UMC_TOKEN_OR) {
        const size_t offset = parser->token.offset;
        advance(parser);
        const size_t right_branch = parser->count;
        emit(parser, UMC_JUMP_IF_ZERO, 0, offset);
        emit(parser, UMC_PUSH, 1, offset);
        const size_t join = parser->count;
        emit(parser, UMC_JUMP, 0, offset);
        patch_forward(parser, right_branch);
        logical_and(parser, depth);
        emit(parser, UMC_PUSH, 0, offset);
        emit(parser, UMC_NOT_EQUAL, 0, offset);
        patch_forward(parser, join);
    }
}

/* Conditional arms recursively parse at one greater depth. This preserves
 * right associativity and prevents a flat chain of ? tokens from bypassing
 * the recursion budget enforced by unary. Both arms compile, but the shared
 * forward runtime executes only the selected one and preserves its value. */
static void expression(UmcParser *parser, size_t depth)
{
    logical_or(parser, depth);
    if (parser->status != UMC_OK || parser->token.kind != UMC_TOKEN_QUESTION) return;
    const size_t offset = parser->token.offset;
    advance(parser);
    const size_t false_branch = parser->count;
    emit(parser, UMC_JUMP_IF_ZERO, 0, offset);
    expression(parser, depth + 1U);
    expect(parser, UMC_TOKEN_COLON);
    const size_t join = parser->count;
    emit(parser, UMC_JUMP, 0, offset);
    patch_forward(parser, false_branch);
    expression(parser, depth + 1U);
    patch_forward(parser, join);
}


/* Initializers execute before their declaration becomes visible. This rejects
 * self/forward reads instead of copying a future or uninitialized slot. Stores
 * consume the expression value, leaving the operand stack empty at statements. */
/* The former local parser reset expression depth at every statement. Retain
 * it for review; the active form receives surrounding branch depth so nested
 * statements and expression recursion share one bounded compile budget. */
#if 0
static void local_statement(UmcParser *parser)
{
    const bool declaration = parser->token.kind == UMC_TOKEN_INT;
    if (declaration) advance(parser);
    if (parser->status != UMC_OK) return;
    const UmcToken name = parser->token;
    if (name.kind != UMC_TOKEN_IDENTIFIER) {
        local_error(parser, name.offset, "expected a local integer name");
        return;
    }
    size_t slot = find_local(parser, &name);
    if (declaration && slot != UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "duplicate local integer declaration");
        return;
    }
    if (!declaration && slot == UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "assignment requires a declared local integer");
        return;
    }
    if (declaration && parser->local_count == UMC_LOCAL_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_LOCAL_CAPACITY, name.offset);
        return;
    }
    /* A local name has no address side effects. Capture its old value once,
     * evaluate the right expression once, then reuse checked arithmetic and
     * the existing store. Declarations still require a plain initializer.
     * The former assignment-only parser is retained for implementation review. */
#if 0
    advance(parser);
    expect(parser, UMC_TOKEN_ASSIGN);
    expression(parser, 0U);
    expect(parser, UMC_TOKEN_SEMICOLON);
#endif
    advance(parser);
    if (parser->status != UMC_OK) return;
    const UmcToken operation = parser->token;
    UmcOpcode arithmetic = UMC_PUSH; /* Sentinel, never emitted for plain =. */
    if (!declaration) {
        switch (operation.kind) {
        case UMC_TOKEN_ADD_ASSIGN: arithmetic = UMC_ADD; break;
        case UMC_TOKEN_SUBTRACT_ASSIGN: arithmetic = UMC_SUBTRACT; break;
        case UMC_TOKEN_MULTIPLY_ASSIGN: arithmetic = UMC_MULTIPLY; break;
        case UMC_TOKEN_DIVIDE_ASSIGN: arithmetic = UMC_DIVIDE; break;
        default: break;
        }
    }
    if (arithmetic == UMC_PUSH) expect(parser, UMC_TOKEN_ASSIGN);
    else {
        advance(parser);
        emit(parser, UMC_LOAD_LOCAL, (int32_t)slot, name.offset);
    }
    expression(parser, 0U);
    if (arithmetic != UMC_PUSH) emit(parser, arithmetic, 0, operation.offset);
    expect(parser, UMC_TOKEN_SEMICOLON);
    if (parser->status != UMC_OK) return;
    if (declaration) {
        slot = parser->local_count++;
        parser->locals[slot].offset = name.offset;
        parser->locals[slot].length = name.length;
    }
    emit(parser, UMC_STORE_LOCAL, (int32_t)slot, name.offset);
}

#endif

/* Remainder assignment loads the old local and evaluates its right operand once before the shared checked operation. The previous implementation is retained for engineering review. */
#if 0
static void local_statement(UmcParser *parser, size_t depth)
{
    const bool declaration = parser->token.kind == UMC_TOKEN_INT;
    if (declaration) advance(parser);
    if (parser->status != UMC_OK) return;
    const UmcToken name = parser->token;
    if (name.kind != UMC_TOKEN_IDENTIFIER) {
        local_error(parser, name.offset, "expected a local integer name");
        return;
    }
    size_t slot = find_local(parser, &name);
    if (declaration && slot != UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "duplicate local integer declaration");
        return;
    }
    if (!declaration && slot == UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "assignment requires a declared local integer");
        return;
    }
    if (declaration && parser->local_count == UMC_LOCAL_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_LOCAL_CAPACITY, name.offset);
        return;
    }
    /* A local name has no address side effects. Capture its old value once,
     * evaluate the right expression once, then reuse checked arithmetic and
     * the existing store. Declarations still require a plain initializer.
     * The former assignment-only parser is retained for implementation review. */
#if 0
    advance(parser);
    expect(parser, UMC_TOKEN_ASSIGN);
    expression(parser, depth);
    expect(parser, UMC_TOKEN_SEMICOLON);
#endif
    advance(parser);
    if (parser->status != UMC_OK) return;
    const UmcToken operation = parser->token;
    UmcOpcode arithmetic = UMC_PUSH; /* Sentinel, never emitted for plain =. */
    if (!declaration) {
        switch (operation.kind) {
        case UMC_TOKEN_ADD_ASSIGN: arithmetic = UMC_ADD; break;
        case UMC_TOKEN_SUBTRACT_ASSIGN: arithmetic = UMC_SUBTRACT; break;
        case UMC_TOKEN_MULTIPLY_ASSIGN: arithmetic = UMC_MULTIPLY; break;
        case UMC_TOKEN_DIVIDE_ASSIGN: arithmetic = UMC_DIVIDE; break;
        default: break;
        }
    }
    if (arithmetic == UMC_PUSH) expect(parser, UMC_TOKEN_ASSIGN);
    else {
        advance(parser);
        emit(parser, UMC_LOAD_LOCAL, (int32_t)slot, name.offset);
    }
    expression(parser, depth);
    if (arithmetic != UMC_PUSH) emit(parser, arithmetic, 0, operation.offset);
    expect(parser, UMC_TOKEN_SEMICOLON);
    if (parser->status != UMC_OK) return;
    if (declaration) {
        slot = parser->local_count++;
        parser->locals[slot].offset = name.offset;
        parser->locals[slot].length = name.length;
    }
    emit(parser, UMC_STORE_LOCAL, (int32_t)slot, name.offset);
}
#endif
static void local_statement(UmcParser *parser, size_t depth)
{
    const bool declaration = parser->token.kind == UMC_TOKEN_INT;
    if (declaration) advance(parser);
    if (parser->status != UMC_OK) return;
    const UmcToken name = parser->token;
    if (name.kind != UMC_TOKEN_IDENTIFIER) {
        local_error(parser, name.offset, "expected a local integer name");
        return;
    }
    size_t slot = find_local(parser, &name);
    if (declaration && slot != UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "duplicate local integer declaration");
        return;
    }
    if (!declaration && slot == UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "assignment requires a declared local integer");
        return;
    }
    if (declaration && parser->local_count == UMC_LOCAL_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_LOCAL_CAPACITY, name.offset);
        return;
    }
    /* A local name has no address side effects. Capture its old value once,
     * evaluate the right expression once, then reuse checked arithmetic and
     * the existing store. Declarations still require a plain initializer.
     * The former assignment-only parser is retained for implementation review. */
#if 0
    advance(parser);
    expect(parser, UMC_TOKEN_ASSIGN);
    expression(parser, depth);
    expect(parser, UMC_TOKEN_SEMICOLON);
#endif
    advance(parser);
    if (parser->status != UMC_OK) return;
    const UmcToken operation = parser->token;
    UmcOpcode arithmetic = UMC_PUSH; /* Sentinel, never emitted for plain =. */
    if (!declaration) {
        switch (operation.kind) {
        case UMC_TOKEN_ADD_ASSIGN: arithmetic = UMC_ADD; break;
        case UMC_TOKEN_SUBTRACT_ASSIGN: arithmetic = UMC_SUBTRACT; break;
        case UMC_TOKEN_MULTIPLY_ASSIGN: arithmetic = UMC_MULTIPLY; break;
        case UMC_TOKEN_DIVIDE_ASSIGN: arithmetic = UMC_DIVIDE; break;
        case UMC_TOKEN_REMAINDER_ASSIGN: arithmetic = UMC_REMAINDER; break;
        case UMC_TOKEN_BITWISE_AND_ASSIGN: arithmetic = UMC_BITWISE_AND; break;
        case UMC_TOKEN_BITWISE_XOR_ASSIGN: arithmetic = UMC_BITWISE_XOR; break;
        case UMC_TOKEN_BITWISE_OR_ASSIGN: arithmetic = UMC_BITWISE_OR; break;
        default: break;
        }
    }
    if (arithmetic == UMC_PUSH) expect(parser, UMC_TOKEN_ASSIGN);
    else {
        advance(parser);
        emit(parser, UMC_LOAD_LOCAL, (int32_t)slot, name.offset);
    }
    expression(parser, depth);
    if (arithmetic != UMC_PUSH) emit(parser, arithmetic, 0, operation.offset);
    expect(parser, UMC_TOKEN_SEMICOLON);
    if (parser->status != UMC_OK) return;
    if (declaration) {
        slot = parser->local_count++;
        parser->locals[slot].offset = name.offset;
        parser->locals[slot].length = name.length;
    }
    emit(parser, UMC_STORE_LOCAL, (int32_t)slot, name.offset);
}

/* Braces delimit control-flow bodies without introducing new local scope.
 * Only already-initialized local updates and nested conditionals are admitted.
 * Both arms are checked at compile time; only the selected arm executes. */
static void if_statement(UmcParser *parser, size_t depth);
static void if_body(UmcParser *parser, size_t depth)
{
    expect(parser, UMC_TOKEN_OPEN);
    while (parser->status == UMC_OK) {
        if (parser->token.kind == UMC_TOKEN_IDENTIFIER) local_statement(parser, depth);
        else if (parser->token.kind == UMC_TOKEN_IF) if_statement(parser, depth);
        else break;
    }
    expect(parser, UMC_TOKEN_CLOSE);
}
/* Admit else-if chains through the existing false branch and shared depth budget; no new opcode or runtime contract is introduced. The previous implementation is retained for engineering review. */
#if 0
static void if_statement(UmcParser *parser, size_t depth)
{
    if (parser->status != UMC_OK) return;
    const size_t offset = parser->token.offset;
    if (depth >= UMC_PARSE_DEPTH_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_DEPTH_LIMIT, offset);
        return;
    }
    expect(parser, UMC_TOKEN_IF);
    expect(parser, UMC_TOKEN_LEFT);
    expression(parser, depth + 1U);
    expect(parser, UMC_TOKEN_RIGHT);
    const size_t false_branch = parser->count;
    emit(parser, UMC_JUMP_IF_ZERO, 0, offset);
    if_body(parser, depth + 1U);
    if (parser->status != UMC_OK) return;
    if (parser->token.kind == UMC_TOKEN_ELSE) {
        const size_t else_offset = parser->token.offset;
        advance(parser);
        const size_t join = parser->count;
        emit(parser, UMC_JUMP, 0, else_offset);
        patch_forward(parser, false_branch);
        if_body(parser, depth + 1U);
        patch_forward(parser, join);
    } else patch_forward(parser, false_branch);
}
#endif
static void if_statement(UmcParser *parser, size_t depth)
{
    if (parser->status != UMC_OK) return;
    const size_t offset = parser->token.offset;
    if (depth >= UMC_PARSE_DEPTH_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_DEPTH_LIMIT, offset);
        return;
    }
    expect(parser, UMC_TOKEN_IF);
    expect(parser, UMC_TOKEN_LEFT);
    expression(parser, depth + 1U);
    expect(parser, UMC_TOKEN_RIGHT);
    const size_t false_branch = parser->count;
    emit(parser, UMC_JUMP_IF_ZERO, 0, offset);
    if_body(parser, depth + 1U);
    if (parser->status != UMC_OK) return;
    if (parser->token.kind == UMC_TOKEN_ELSE) {
        const size_t else_offset = parser->token.offset;
        advance(parser);
        const size_t join = parser->count;
        emit(parser, UMC_JUMP, 0, else_offset);
        patch_forward(parser, false_branch);
        /* An else-if is the false path of this branch. Charge another parse
         * level before descending, just as a nested conditional in braces does.
         * Earlier selected bodies jump beyond the complete remaining chain. */
        if (parser->token.kind == UMC_TOKEN_IF) if_statement(parser, depth + 1U);
        else if_body(parser, depth + 1U);
        patch_forward(parser, join);
    } else patch_forward(parser, false_branch);
}

/* Compile initialized declarations and assignments before the final return using one per-call symbol table. The predecessor remains disabled for review. */
#if 0
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

#endif

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
    /* Preserve the straight-line statement loop for comparison. The active
     * dispatcher keeps its declarations/assignments and adds braced if/else
     * updates before the same required final return expression. */
#if 0
    while (parser.status == UMC_OK &&
        (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_IDENTIFIER))
        local_statement(&parser);
#endif
    while (parser.status == UMC_OK) {
        if (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_IDENTIFIER)
            local_statement(&parser, 0U);
        else if (parser.token.kind == UMC_TOKEN_IF) if_statement(&parser, 0U);
        else break;
    }
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


/* Source grammar belongs to the compiler. In particular ! is a lowering rule,
 * not a runtime opcode, so the runtime must not advertise it as its own feature. */
/* Report local grammar capability from the linked compiler, including its actual slot and identifier bounds. The predecessor remains disabled for review. */
#if 0
uint32_t umc_compiler_profile(UmcProfileProperty property)
{
    switch (property) {
    case UMC_PROFILE_API: return UMC_API_VERSION;
    case UMC_PROFILE_FEATURES:
        /* Retain the earlier mask for review. Literal spelling is reported by
         * this compiler, while the runtime continues reporting operations. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT;
#endif
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS;
    case UMC_PROFILE_SOURCE_BYTES: return UMC_SOURCE_LIMIT;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_PARSE_DEPTH: return UMC_PARSE_DEPTH_LIMIT;
    default: return 0U;
    }
}

#endif

/* Advertise the source operator extension separately from the runtime branch instruction set. The predecessor remains disabled for review. */
#if 0
uint32_t umc_compiler_profile(UmcProfileProperty property)
{
    switch (property) {
    case UMC_PROFILE_API: return UMC_API_VERSION;
    case UMC_PROFILE_FEATURES:
        /* Retain the earlier mask for review. Literal spelling is reported by
         * this compiler, while the runtime continues reporting operations. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT;
#endif
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS;
    case UMC_PROFILE_LOCAL_VALUES: return UMC_LOCAL_LIMIT;
    case UMC_PROFILE_IDENTIFIER_BYTES: return UMC_IDENTIFIER_LIMIT;
    case UMC_PROFILE_SOURCE_BYTES: return UMC_SOURCE_LIMIT;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_PARSE_DEPTH: return UMC_PARSE_DEPTH_LIMIT;
    default: return 0U;
    }
}

#endif

/* Advertise the conditional grammar only from the linked compiler; no runtime opcode is added. The predecessor remains disabled for review. */
#if 0
uint32_t umc_compiler_profile(UmcProfileProperty property)
{
    switch (property) {
    case UMC_PROFILE_API: return UMC_API_VERSION;
    case UMC_PROFILE_FEATURES:
        /* Retain the earlier mask for review. Literal spelling is reported by
         * this compiler, while the runtime continues reporting operations. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT;
#endif
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES;
    case UMC_PROFILE_LOCAL_VALUES: return UMC_LOCAL_LIMIT;
    case UMC_PROFILE_IDENTIFIER_BYTES: return UMC_IDENTIFIER_LIMIT;
    case UMC_PROFILE_SOURCE_BYTES: return UMC_SOURCE_LIMIT;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_PARSE_DEPTH: return UMC_PARSE_DEPTH_LIMIT;
    default: return 0U;
    }
}

#endif

/* Report the linked compiler remainder capability rather than inferring it from a new header. The predecessor remains disabled for review. */
#if 0
uint32_t umc_compiler_profile(UmcProfileProperty property)
{
    switch (property) {
    case UMC_PROFILE_API: return UMC_API_VERSION;
    case UMC_PROFILE_FEATURES:
        /* Retain the earlier mask for review. Literal spelling is reported by
         * this compiler, while the runtime continues reporting operations. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT;
#endif
        /* Compound assignment is source lowering, not a new runtime opcode.
         * Retain the previous capability report as the historical boundary. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS;
#endif
        /* Preserve the prior compiler mask; chained spelling is source-only. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS;
#endif
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS |
            UMC_FEATURE_ELSE_IF_CHAINS;
    case UMC_PROFILE_LOCAL_VALUES: return UMC_LOCAL_LIMIT;
    case UMC_PROFILE_IDENTIFIER_BYTES: return UMC_IDENTIFIER_LIMIT;
    case UMC_PROFILE_SOURCE_BYTES: return UMC_SOURCE_LIMIT;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_PARSE_DEPTH: return UMC_PARSE_DEPTH_LIMIT;
    default: return 0U;
    }
}
#endif

uint32_t umc_compiler_profile(UmcProfileProperty property)
{
    switch (property) {
    case UMC_PROFILE_API: return UMC_API_VERSION;
    case UMC_PROFILE_FEATURES:
        /* Retain the earlier mask for review. Literal spelling is reported by
         * this compiler, while the runtime continues reporting operations. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT;
#endif
        /* Compound assignment is source lowering, not a new runtime opcode.
         * Retain the previous capability report as the historical boundary. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS;
#endif
        /* Preserve the prior compiler mask; chained spelling is source-only. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS;
#endif
        /* Source complement is additive; runtime and wire capabilities remain unchanged. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS |
            UMC_FEATURE_ELSE_IF_CHAINS | UMC_FEATURE_REMAINDER;
#endif
        /* Preserve the prior source feature set; binary bitwise is additive. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS |
            UMC_FEATURE_ELSE_IF_CHAINS | UMC_FEATURE_REMAINDER | UMC_FEATURE_BITWISE_COMPLEMENT;
#endif
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS |
            UMC_FEATURE_ELSE_IF_CHAINS | UMC_FEATURE_REMAINDER | UMC_FEATURE_BITWISE_COMPLEMENT |
            UMC_FEATURE_BINARY_BITWISE;
    case UMC_PROFILE_LOCAL_VALUES: return UMC_LOCAL_LIMIT;
    case UMC_PROFILE_IDENTIFIER_BYTES: return UMC_IDENTIFIER_LIMIT;
    case UMC_PROFILE_SOURCE_BYTES: return UMC_SOURCE_LIMIT;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_PARSE_DEPTH: return UMC_PARSE_DEPTH_LIMIT;
    default: return 0U;
    }
}
