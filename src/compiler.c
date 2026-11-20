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
#include "umicom_c/int_input.h"
#include "umicom_c/profile.h"
#include "umicom_c/source_check.h"

/* This optional stack-owned collector belongs to one source check. Ordinary
 * compilation passes NULL and acquires no warning array. Declaration identities
 * are distinct from VM slots, which are reused after lexical scope exit. */
typedef struct UmcWarningState {
    UmcSourceWarning declarations[UMC_SOURCE_WARNING_LIMIT];
    bool read[UMC_SOURCE_WARNING_LIMIT];
    size_t count;
} UmcWarningState;

typedef struct UmcParser {
    UmcLexer lexer;
    UmcToken token;
    UmcInstruction *output;
    size_t capacity;
    size_t count;
    UmcDiagnostic *diagnostic;
    UmcStatus status;
/* Keep read-only source qualification in the compiler symbol table, without changing runtime slots or portable bytecode. The former implementation is retained for owner review. */
#if 0
    struct { size_t offset; size_t length; } locals[UMC_LOCAL_LIMIT];
#endif
/* Keep warning declaration identities separate from reused runtime slots while preserving the source qualifier and name ownership. The former implementation is retained for owner review. */
#if 0
    struct { size_t offset; size_t length; bool read_only; } locals[UMC_LOCAL_LIMIT];
#endif
    struct { size_t offset; size_t length; bool read_only; size_t warning_index; } locals[UMC_LOCAL_LIMIT];
    UmcWarningState *warnings;
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
    /* All source reads lower through LOAD, including compound updates and
     * prefix/postfix statements. Observe the accepted instruction once here. */
    if (opcode == UMC_LOAD_LOCAL && parser->warnings != NULL && operand >= 0 &&
        (size_t)operand < parser->local_count) {
        const size_t index = parser->locals[(size_t)operand].warning_index;
        if (index < parser->warnings->count) parser->warnings->read[index] = true;
    }
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
/* Route comparisons through checked shifts while retaining the earlier precedence layer for review. The former implementation is retained for owner review. */
#if 0
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

#endif
/* A distinct precedence layer keeps addition inside each shift operand.
 * Iteration preserves left association without extra recursive depth. */
static void shift_expression(UmcParser *parser, size_t depth)
{
    additive(parser, depth);
    while (parser->status == UMC_OK &&
        (parser->token.kind == UMC_TOKEN_SHIFT_LEFT ||
         parser->token.kind == UMC_TOKEN_SHIFT_RIGHT)) {
        const UmcToken token = parser->token;
        advance(parser);
        additive(parser, depth);
        emit(parser, token.kind == UMC_TOKEN_SHIFT_LEFT ? UMC_SHIFT_LEFT :
             UMC_SHIFT_RIGHT, 0, token.offset);
    }
}

static void relational(UmcParser *parser, size_t depth)
{
    shift_expression(parser, depth);
    while (parser->status == UMC_OK &&
        (parser->token.kind == UMC_TOKEN_LESS || parser->token.kind == UMC_TOKEN_LESS_EQUAL ||
         parser->token.kind == UMC_TOKEN_GREATER || parser->token.kind == UMC_TOKEN_GREATER_EQUAL)) {
        const UmcToken token = parser->token;
        advance(parser);
        shift_expression(parser, depth);
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
/* Statement-only updates have no expression result to retain. Resolve a single
 * initialized local, then reuse checked arithmetic and the validated store.
 * Both prefix and postfix forms evaluate the same one update exactly once. */
static void emit_increment(UmcParser *parser, size_t slot, const UmcToken *operation)
{
    emit(parser, UMC_LOAD_LOCAL, (int32_t)slot, operation->offset);
    emit(parser, UMC_PUSH, 1, operation->offset);
    emit(parser, operation->kind == UMC_TOKEN_INCREMENT ? UMC_ADD : UMC_SUBTRACT,
         0, operation->offset);
    emit(parser, UMC_STORE_LOCAL, (int32_t)slot, operation->offset);
}
/* Prefix updates share the same source mutability check as assignments and postfix updates; no runtime update is emitted for a read-only name. The former implementation is retained for owner review. */
#if 0
static void prefix_increment_statement(UmcParser *parser)
{
    const UmcToken operation = parser->token;
    advance(parser);
    if (parser->status != UMC_OK) return;
    const UmcToken name = parser->token;
    if (name.kind != UMC_TOKEN_IDENTIFIER) {
        local_error(parser, name.offset, "increment requires an initialized local name");
        return;
    }
    const size_t slot = find_local(parser, &name);
    if (slot == UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "increment requires a declared local integer");
        return;
    }
    advance(parser);
    expect(parser, UMC_TOKEN_SEMICOLON);
    if (parser->status == UMC_OK) emit_increment(parser, slot, &operation);
}

#endif
static void prefix_increment_statement(UmcParser *parser)
{
    const UmcToken operation = parser->token;
    advance(parser);
    if (parser->status != UMC_OK) return;
    const UmcToken name = parser->token;
    if (name.kind != UMC_TOKEN_IDENTIFIER) {
        local_error(parser, name.offset, "increment requires an initialized local name");
        return;
    }
    const size_t slot = find_local(parser, &name);
    if (slot == UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "increment requires a declared local integer");
        return;
    }
    if (parser->locals[slot].read_only) {
        local_error(parser, name.offset, "cannot update a read-only local integer");
        return;
    }
    advance(parser);
    expect(parser, UMC_TOKEN_SEMICOLON);
    if (parser->status == UMC_OK) emit_increment(parser, slot, &operation);
}

/* Admit one const qualifier around int and reject all assignments to qualified locals before lowering. Initializer visibility, bounds and existing updates remain unchanged. The former implementation is retained for owner review. */
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
    if (!declaration && (operation.kind == UMC_TOKEN_INCREMENT ||
                         operation.kind == UMC_TOKEN_DECREMENT)) {
        advance(parser);
        expect(parser, UMC_TOKEN_SEMICOLON);
        if (parser->status == UMC_OK) emit_increment(parser, slot, &operation);
        return;
    }
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

#endif
/* Separate declaration-list ownership from existing update statements. The
 * previous combined parser is retained for review; its single declarations,
 * qualifiers, slot limits and update operations remain supported below. */
#if 0
static void local_statement(UmcParser *parser, size_t depth)
{
    const bool declaration = parser->token.kind == UMC_TOKEN_INT ||
                             parser->token.kind == UMC_TOKEN_CONST;
    bool read_only = false;
    if (parser->token.kind == UMC_TOKEN_CONST) {
        read_only = true;
        advance(parser);
        expect(parser, UMC_TOKEN_INT);
    } else if (declaration) {
        advance(parser);
        if (parser->token.kind == UMC_TOKEN_CONST) {
            read_only = true;
            advance(parser);
        }
    }
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
    if (!declaration && parser->locals[slot].read_only) {
        local_error(parser, name.offset, "cannot update a read-only local integer");
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
    if (!declaration && (operation.kind == UMC_TOKEN_INCREMENT ||
                         operation.kind == UMC_TOKEN_DECREMENT)) {
        advance(parser);
        expect(parser, UMC_TOKEN_SEMICOLON);
        if (parser->status == UMC_OK) emit_increment(parser, slot, &operation);
        return;
    }
    UmcOpcode arithmetic = UMC_PUSH; /* Sentinel, never emitted for plain =. */
    if (!declaration) {
        switch (operation.kind) {
        case UMC_TOKEN_ADD_ASSIGN: arithmetic = UMC_ADD; break;
        case UMC_TOKEN_SUBTRACT_ASSIGN: arithmetic = UMC_SUBTRACT; break;
        case UMC_TOKEN_MULTIPLY_ASSIGN: arithmetic = UMC_MULTIPLY; break;
        case UMC_TOKEN_DIVIDE_ASSIGN: arithmetic = UMC_DIVIDE; break;
        case UMC_TOKEN_REMAINDER_ASSIGN: arithmetic = UMC_REMAINDER; break;
        case UMC_TOKEN_SHIFT_LEFT_ASSIGN: arithmetic = UMC_SHIFT_LEFT; break;
        case UMC_TOKEN_SHIFT_RIGHT_ASSIGN: arithmetic = UMC_SHIFT_RIGHT; break;
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
        parser->locals[slot].read_only = read_only;
        if (parser->warnings != NULL) {
            UmcWarningState *warnings = parser->warnings;
            if (warnings->count == UMC_SOURCE_WARNING_LIMIT) {
                parser->status = umc_fail(parser->diagnostic, UMC_PROGRAM_TOO_LARGE, name.offset);
                return;
            }
            parser->locals[slot].warning_index = warnings->count;
            warnings->declarations[warnings->count] = (UmcSourceWarning){
                .kind = UMC_WARNING_LOCAL_NEVER_READ,
                .source_offset = name.offset, .source_length = name.length
            };
            ++warnings->count;
        }
    }
    emit(parser, UMC_STORE_LOCAL, (int32_t)slot, name.offset);
}

#endif
/* A declaration list shares its type/qualifier, but each initializer owns a
 * separate sequence point in the emitted program. Register a name only after
 * its initializer compiles; later declarators can read it without permitting
 * self-reference, active-name shadowing or a comma expression. */
static void local_declarator(UmcParser *parser, size_t depth, bool read_only)
{
    if(parser->status!=UMC_OK) return;
    const UmcToken name=parser->token;
    if(name.kind!=UMC_TOKEN_IDENTIFIER) {
        local_error(parser,name.offset,"expected a local integer name");
        return;
    }
    if(find_local(parser,&name)!=UMC_LOCAL_LIMIT) {
        local_error(parser,name.offset,"duplicate local integer declaration");
        return;
    }
    if(parser->local_count==UMC_LOCAL_LIMIT) {
        parser->status=umc_fail(parser->diagnostic,UMC_LOCAL_CAPACITY,name.offset);
        return;
    }
    advance(parser);
    expect(parser,UMC_TOKEN_ASSIGN);
    expression(parser,depth);
    if(parser->status!=UMC_OK) return;
    if(parser->token.kind!=UMC_TOKEN_COMMA && parser->token.kind!=UMC_TOKEN_SEMICOLON) {
        parser->status=umc_fail(parser->diagnostic,UMC_SYNTAX_ERROR,parser->token.offset);
        return;
    }
    if(parser->warnings!=NULL && parser->warnings->count==UMC_SOURCE_WARNING_LIMIT) {
        parser->status=umc_fail(parser->diagnostic,UMC_PROGRAM_TOO_LARGE,name.offset);
        return;
    }
    const size_t slot=parser->local_count++;
    parser->locals[slot].offset=name.offset;
    parser->locals[slot].length=name.length;
    parser->locals[slot].read_only=read_only;
    if(parser->warnings!=NULL) {
        UmcWarningState *warnings=parser->warnings;
        parser->locals[slot].warning_index=warnings->count;
        warnings->declarations[warnings->count]=(UmcSourceWarning){
            .kind=UMC_WARNING_LOCAL_NEVER_READ,
            .source_offset=name.offset,.source_length=name.length
        };
        ++warnings->count;
    }
    emit(parser,UMC_STORE_LOCAL,(int32_t)slot,name.offset);
}
static void local_declaration(UmcParser *parser, size_t depth)
{
    bool read_only=false;
    if(parser->token.kind==UMC_TOKEN_CONST) {
        read_only=true;
        advance(parser);
        expect(parser,UMC_TOKEN_INT);
    } else {
        expect(parser,UMC_TOKEN_INT);
        if(parser->token.kind==UMC_TOKEN_CONST) {read_only=true;advance(parser);}
    }
    while(parser->status==UMC_OK) {
        local_declarator(parser,depth,read_only);
        if(parser->status!=UMC_OK) return;
        if(parser->token.kind!=UMC_TOKEN_COMMA) break;
        advance(parser);
    }
    expect(parser,UMC_TOKEN_SEMICOLON);
}
static void local_statement(UmcParser *parser, size_t depth)
{
    if(parser->token.kind==UMC_TOKEN_INT || parser->token.kind==UMC_TOKEN_CONST) {
        local_declaration(parser,depth);
        return;
    }
    const UmcToken name=parser->token;
    if(name.kind!=UMC_TOKEN_IDENTIFIER) {
        local_error(parser,name.offset,"expected a local integer name");
        return;
    }
    const size_t slot=find_local(parser,&name);
    if(slot==UMC_LOCAL_LIMIT) {
        local_error(parser,name.offset,"assignment requires a declared local integer");
        return;
    }
    if(parser->locals[slot].read_only) {
        local_error(parser,name.offset,"cannot update a read-only local integer");
        return;
    }
    advance(parser);
    if(parser->status!=UMC_OK) return;
    const UmcToken operation=parser->token;
    if(operation.kind==UMC_TOKEN_INCREMENT || operation.kind==UMC_TOKEN_DECREMENT) {
        advance(parser);
        expect(parser,UMC_TOKEN_SEMICOLON);
        if(parser->status==UMC_OK)emit_increment(parser,slot,&operation);
        return;
    }
    /* Retain one load, one right-hand expression and the existing checked
     * arithmetic/store semantics for every previously supported update. */
    UmcOpcode arithmetic=UMC_PUSH;
    switch(operation.kind) {
    case UMC_TOKEN_ADD_ASSIGN: arithmetic=UMC_ADD;break;
    case UMC_TOKEN_SUBTRACT_ASSIGN: arithmetic=UMC_SUBTRACT;break;
    case UMC_TOKEN_MULTIPLY_ASSIGN: arithmetic=UMC_MULTIPLY;break;
    case UMC_TOKEN_DIVIDE_ASSIGN: arithmetic=UMC_DIVIDE;break;
    case UMC_TOKEN_REMAINDER_ASSIGN: arithmetic=UMC_REMAINDER;break;
    case UMC_TOKEN_SHIFT_LEFT_ASSIGN: arithmetic=UMC_SHIFT_LEFT;break;
    case UMC_TOKEN_SHIFT_RIGHT_ASSIGN: arithmetic=UMC_SHIFT_RIGHT;break;
    case UMC_TOKEN_BITWISE_AND_ASSIGN: arithmetic=UMC_BITWISE_AND;break;
    case UMC_TOKEN_BITWISE_XOR_ASSIGN: arithmetic=UMC_BITWISE_XOR;break;
    case UMC_TOKEN_BITWISE_OR_ASSIGN: arithmetic=UMC_BITWISE_OR;break;
    default:break;
    }
    if(arithmetic==UMC_PUSH)expect(parser,UMC_TOKEN_ASSIGN);
    else {advance(parser);emit(parser,UMC_LOAD_LOCAL,(int32_t)slot,name.offset);}
    expression(parser,depth);
    if(arithmetic!=UMC_PUSH)emit(parser,arithmetic,0,operation.offset);
    expect(parser,UMC_TOKEN_SEMICOLON);
    if(parser->status==UMC_OK)emit(parser,UMC_STORE_LOCAL,(int32_t)slot,name.offset);
}

/* Braces delimit control-flow bodies without introducing new local scope.
 * Only already-initialized local updates and nested conditionals are admitted.
 * Both arms are checked at compile time; only the selected arm executes. */
/* The description above records the original update-only grammar. The active
 * body owner below also admits scoped declarations and standalone blocks;
 * both selected and unselected source paths are still checked. */
static void if_statement(UmcParser *parser, size_t depth);
static void block_statement(UmcParser *parser, size_t depth);
static void counted_for_statement(UmcParser *parser, size_t depth);

static bool is_counted_for(const UmcParser *parser)
{
    /* Recognize the statement as a unit. Existing source can still use the
     * spelling "for" as a local where it was previously an ordinary name. */
    const UmcToken token = parser->token;
    if (token.kind != UMC_TOKEN_IDENTIFIER || token.length != 3U) return false;
    const char *text = parser->lexer.source + token.offset;
    if (text[0] != 'f' || text[1] != 'o' || text[2] != 'r') return false;
    UmcLexer lookahead = parser->lexer;
    UmcToken next;
    return umc_lex_next(&lookahead, &next, NULL) == UMC_OK && next.kind == UMC_TOKEN_LEFT;
}

static bool statement(UmcParser *parser, size_t depth)
{
    /* Use the same statement vocabulary at the entry and inside braces.
     * The caller retains ownership of its scope and terminating token. */
    if (is_counted_for(parser)) counted_for_statement(parser, depth);
    else if (parser->token.kind == UMC_TOKEN_IDENTIFIER ||
             parser->token.kind == UMC_TOKEN_INT || parser->token.kind == UMC_TOKEN_CONST)
        local_statement(parser, depth);
    else if (parser->token.kind == UMC_TOKEN_INCREMENT || parser->token.kind == UMC_TOKEN_DECREMENT)
        prefix_increment_statement(parser);
    else if (parser->token.kind == UMC_TOKEN_IF) if_statement(parser, depth);
    else if (parser->token.kind == UMC_TOKEN_OPEN) block_statement(parser, depth);
    else if (parser->token.kind == UMC_TOKEN_SEMICOLON) advance(parser);
    else return false;
    return true;
}
/* Conditional blocks now own bounded lexical names. Save and restore the visible symbol count around the existing statement parser, retaining checked initialization, read-only qualifiers and runtime control-flow admission. The earlier update-only body remains for owner review. The former implementation is retained for owner review. */
#if 0
static void if_body(UmcParser *parser, size_t depth)
{
    expect(parser, UMC_TOKEN_OPEN);
    while (parser->status == UMC_OK) {
/* Conditional bodies also admit the same checked prefix update statement. The former implementation is retained for owner review. */
#if 0
        if (parser->token.kind == UMC_TOKEN_IDENTIFIER) local_statement(parser, depth);
#endif
        if (parser->token.kind == UMC_TOKEN_IDENTIFIER) local_statement(parser, depth);
        else if (parser->token.kind == UMC_TOKEN_INCREMENT || parser->token.kind == UMC_TOKEN_DECREMENT)
            prefix_increment_statement(parser);
        else if (parser->token.kind == UMC_TOKEN_IF) if_statement(parser, depth);
        else break;
    }
    expect(parser, UMC_TOKEN_CLOSE);
}
#endif
static void if_body(UmcParser *parser, size_t depth)
{
    const size_t outer_count = parser->local_count;
    expect(parser, UMC_TOKEN_OPEN);
    /* The common statement dispatcher now admits the same counted loop
     * inside every scope. Retain the preceding body dispatcher for review. */
#if 0
    while (parser->status == UMC_OK) {
        if (parser->token.kind == UMC_TOKEN_IDENTIFIER ||
            parser->token.kind == UMC_TOKEN_INT || parser->token.kind == UMC_TOKEN_CONST)
            local_statement(parser, depth);
        else if (parser->token.kind == UMC_TOKEN_INCREMENT || parser->token.kind == UMC_TOKEN_DECREMENT)
            prefix_increment_statement(parser);
        else if (parser->token.kind == UMC_TOKEN_IF) if_statement(parser, depth);
        else if (parser->token.kind == UMC_TOKEN_OPEN) block_statement(parser, depth);
        /* An empty source statement consumes text, not runtime fuel or slots. */
        else if (parser->token.kind == UMC_TOKEN_SEMICOLON) advance(parser);
        else break;
    }
#endif
    while (parser->status == UMC_OK && statement(parser, depth)) {}
    expect(parser, UMC_TOKEN_CLOSE);
    /* Lexical names end at the brace, even when compilation has failed.
     * Later disjoint scopes may reuse these VM slots only through another
     * initialized declaration. Active-name shadowing remains unsupported. */
    parser->local_count = outer_count;
}
/* A standalone compound statement uses the same lexical owner as a branch
 * body. Charge one parse level before descending, even for an empty body;
 * braces emit no VM operations and do not clear or release local storage. */
static void block_statement(UmcParser *parser, size_t depth)
{
    if (parser->status != UMC_OK) return;
    if (depth >= UMC_PARSE_DEPTH_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_DEPTH_LIMIT, parser->token.offset);
        return;
    }
    if_body(parser, depth + 1U);
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

static int32_t counted_for_literal(UmcParser *parser)
{
    if (parser->status != UMC_OK) return 0;
    const UmcToken token = parser->token;
    /* Integer tokens also carry character and Boolean values. A loop range
     * requires a numeric spelling that begins with a digit. */
    if (token.kind != UMC_TOKEN_INTEGER || parser->lexer.source[token.offset] < '0' ||
        parser->lexer.source[token.offset] > '9') {
        local_error(parser, token.offset, "expected a nonnegative loop-bound integer literal");
        return 0;
    }
    advance(parser);
    return token.integer;
}

static void counted_for_name(UmcParser *parser, const UmcToken *name)
{
    if (parser->status != UMC_OK) return;
    const UmcToken current = parser->token;
    if (current.kind != UMC_TOKEN_IDENTIFIER || current.length != name->length) {
        local_error(parser, current.offset, "loop header must use the declared loop integer");
        return;
    }
    /* The repeated header name must identify the exact declaration above. */
    for (size_t i = 0U; i < name->length; ++i) {
        if (parser->lexer.source[current.offset + i] != parser->lexer.source[name->offset + i]) {
            local_error(parser, current.offset, "loop header must use the declared loop integer");
            return;
        }
    }
    advance(parser);
}

static void counted_for_statement(UmcParser *parser, size_t depth)
{
    const size_t offset = parser->token.offset;
    if (depth >= UMC_PARSE_DEPTH_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_DEPTH_LIMIT, offset);
        return;
    }
    advance(parser);
    expect(parser, UMC_TOKEN_LEFT);
    expect(parser, UMC_TOKEN_INT);
    const UmcToken name = parser->token;
    expect(parser, UMC_TOKEN_IDENTIFIER);
    if (parser->status != UMC_OK) return;
    if (find_local(parser, &name) != UMC_LOCAL_LIMIT) {
        local_error(parser, name.offset, "duplicate local integer declaration");
        return;
    }
    if (parser->local_count == UMC_LOCAL_LIMIT) {
        parser->status = umc_fail(parser->diagnostic, UMC_LOCAL_CAPACITY, name.offset);
        return;
    }
    expect(parser, UMC_TOKEN_ASSIGN);
    const int32_t first = counted_for_literal(parser);
    expect(parser, UMC_TOKEN_SEMICOLON);
    counted_for_name(parser, &name);
    expect(parser, UMC_TOKEN_LESS);
    const size_t limit_offset = parser->token.offset;
    const int32_t limit = counted_for_literal(parser);
    expect(parser, UMC_TOKEN_SEMICOLON);
    expect(parser, UMC_TOKEN_INCREMENT);
    counted_for_name(parser, &name);
    expect(parser, UMC_TOKEN_RIGHT);
    if (parser->status != UMC_OK) return;
    if (limit <= first || (uint32_t)(limit - first) > UMC_COUNTED_FOR_LIMIT) {
        local_error(parser, limit_offset, "counted for must run from one to sixteen iterations");
        return;
    }

    /* The header owns the induction integer. A body may read it, while the
     * existing read-only rule refuses a write that would change the range. */
    const size_t slot = parser->local_count++;
    parser->locals[slot].offset = name.offset;
    parser->locals[slot].length = name.length;
    parser->locals[slot].read_only = true;
    parser->locals[slot].warning_index = SIZE_MAX; /* The header already uses this name. */
    emit(parser, UMC_PUSH, first, name.offset);
    emit(parser, UMC_STORE_LOCAL, (int32_t)slot, name.offset);
    const size_t body_start = parser->count;
    if_body(parser, depth + 1U);
    const size_t body_end = parser->count;

    /* Parse the body once so warnings and lexical identities are not
     * repeated. Copy only its emitted instructions, rebasing its existing
     * forward branches to equivalent instructions in the new iteration. */
    for (int32_t value = first + 1; parser->status == UMC_OK && value < limit; ++value) {
        emit(parser, UMC_PUSH, value, name.offset);
        emit(parser, UMC_STORE_LOCAL, (int32_t)slot, name.offset);
        const int32_t shift = (int32_t)(parser->count - body_start);
        for (size_t index = body_start; parser->status == UMC_OK && index < body_end; ++index) {
            UmcInstruction copy = parser->output[index];
            if (copy.opcode == UMC_JUMP || copy.opcode == UMC_JUMP_IF_ZERO) copy.operand += shift;
            emit(parser, copy.opcode, copy.operand, copy.source_offset);
        }
    }
    parser->local_count = slot;
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

/* Share one parser and bytecode validator between ordinary compilation and optional source warnings. The former public implementation is retained for review; its public entry now delegates to this owner. The former implementation is retained for owner review. */
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
    /* Preserve the straight-line statement loop for comparison. The active
     * dispatcher keeps its declarations/assignments and adds braced if/else
     * updates before the same required final return expression. */
#if 0
    while (parser.status == UMC_OK &&
        (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_IDENTIFIER))
        local_statement(&parser);
#endif
    while (parser.status == UMC_OK) {
/* Top-level updates reuse the initialized local owner and keep the required final return. The former implementation is retained for owner review. */
#if 0
        if (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_IDENTIFIER)
            local_statement(&parser, 0U);
#endif
/* Top-level initialized locals now admit an explicit read-only qualifier; conditional bodies still exclude declarations. The former implementation is retained for owner review. */
#if 0
        if (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_IDENTIFIER)
            local_statement(&parser, 0U);
#endif
        if (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_CONST ||
            parser.token.kind == UMC_TOKEN_IDENTIFIER)
            local_statement(&parser, 0U);
        else if (parser.token.kind == UMC_TOKEN_INCREMENT || parser.token.kind == UMC_TOKEN_DECREMENT)
            prefix_increment_statement(&parser);
        else if (parser.token.kind == UMC_TOKEN_IF) if_statement(&parser, 0U);
        else if (parser.token.kind == UMC_TOKEN_OPEN) block_statement(&parser, 0U);
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
#endif
static void int_input_entry(UmcParser *parser)
{
    static const char entry_name[] = "umicom_run";
    if (parser->status != UMC_OK) return;
    const UmcToken entry = parser->token;
    if (entry.kind != UMC_TOKEN_IDENTIFIER || entry.length != sizeof entry_name - 1U) {
        local_error(parser, entry.offset, "expected the umicom_run entry point");
        return;
    }
    for (size_t i = 0U; i < entry.length; ++i) {
        if (parser->lexer.source[entry.offset + i] != entry_name[i]) {
            local_error(parser, entry.offset, "expected the umicom_run entry point");
            return;
        }
    }
    advance(parser);
    expect(parser, UMC_TOKEN_LEFT);
    expect(parser, UMC_TOKEN_INT);
    const UmcToken parameter = parser->token;
    expect(parser, UMC_TOKEN_IDENTIFIER);
    expect(parser, UMC_TOKEN_RIGHT);
    if (parser->status != UMC_OK) return;

    /* Parameter references use the existing local binding. The explicit input
     * artifact records that the first push must be bound by its host. */
    parser->locals[0].offset = parameter.offset;
    parser->locals[0].length = parameter.length;
    parser->locals[0].warning_index = UMC_SOURCE_WARNING_LIMIT;
    parser->local_count = 1U;
    emit(parser, UMC_PUSH, 0, parameter.offset);
    emit(parser, UMC_STORE_LOCAL, 0, parameter.offset);
}

static UmcStatus compile_source(const char *source, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic,
    UmcWarningState *warnings, bool int_input)
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
        .status = UMC_OK,
        .warnings = warnings
    };
    advance(&parser);
    expect(&parser, UMC_TOKEN_INT);
    if (int_input) int_input_entry(&parser);
    else {
        expect(&parser, UMC_TOKEN_MAIN);
        expect(&parser, UMC_TOKEN_LEFT);
        expect(&parser, UMC_TOKEN_VOID);
        expect(&parser, UMC_TOKEN_RIGHT);
    }
    expect(&parser, UMC_TOKEN_OPEN);
    /* Preserve the straight-line statement loop for comparison. The active
     * dispatcher keeps its declarations/assignments and adds braced if/else
     * updates before the same required final return expression. */
#if 0
    while (parser.status == UMC_OK &&
        (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_IDENTIFIER))
        local_statement(&parser);
#endif
    /* All scopes share the counted-statement dispatcher. Keep this earlier
     * top-level dispatch for reviewing the unchanged statement ordering. */
#if 0
    while (parser.status == UMC_OK) {
/* Top-level updates reuse the initialized local owner and keep the required final return. The former implementation is retained for owner review. */
#if 0
        if (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_IDENTIFIER)
            local_statement(&parser, 0U);
#endif
/* Top-level initialized locals now admit an explicit read-only qualifier; conditional bodies still exclude declarations. The former implementation is retained for owner review. */
#if 0
        if (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_IDENTIFIER)
            local_statement(&parser, 0U);
#endif
        if (parser.token.kind == UMC_TOKEN_INT || parser.token.kind == UMC_TOKEN_CONST ||
            parser.token.kind == UMC_TOKEN_IDENTIFIER)
            local_statement(&parser, 0U);
        else if (parser.token.kind == UMC_TOKEN_INCREMENT || parser.token.kind == UMC_TOKEN_DECREMENT)
            prefix_increment_statement(&parser);
        else if (parser.token.kind == UMC_TOKEN_IF) if_statement(&parser, 0U);
        else if (parser.token.kind == UMC_TOKEN_OPEN) block_statement(&parser, 0U);
        else if (parser.token.kind == UMC_TOKEN_SEMICOLON) advance(&parser);
        else break;
    }
#endif
    while (parser.status == UMC_OK && statement(&parser, 0U)) {}
    const size_t return_offset = parser.token.offset;
    expect(&parser, UMC_TOKEN_RETURN);
    expression(&parser, 0U);
    expect(&parser, UMC_TOKEN_SEMICOLON);
    /* Permit trailing empty statements while keeping the required return as
     * the final nonempty statement. No unreachable expressions are admitted. */
    while (parser.status == UMC_OK && parser.token.kind == UMC_TOKEN_SEMICOLON)
        advance(&parser);
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

UmcStatus umc_compile(const char *source, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    return compile_source(source, length, output, capacity, count, diagnostic, NULL, false);
}

UmcStatus umc_compile_int_input(const char *source, size_t length, uint8_t *output,
    size_t capacity, size_t *size, UmcDiagnostic *diagnostic)
{
    if (size == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    *size = 0U;
    if (output == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    UmcInstruction instructions[UMC_PROGRAM_LIMIT];
    size_t count = 0U;
    UmcStatus status = compile_source(source, length, instructions, UMC_PROGRAM_LIMIT, &count,
        diagnostic, NULL, true);
    if (status != UMC_OK) return status;
    status = umc_encode(instructions, count, output, capacity, size, diagnostic);
    if (status == UMC_OK) output[5] = UMC_INT_INPUT_BYTECODE_FLAG;
    return status;
}

UmcStatus umc_check_source(const char *source, size_t length,
    UmcSourceReport *report, UmcDiagnostic *diagnostic)
{
    if (report == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    UmcInstruction instructions[UMC_PROGRAM_LIMIT];
    UmcWarningState warnings = {0};
    size_t count = 0U;
    UmcStatus status = compile_source(source, length, instructions,
        UMC_PROGRAM_LIMIT, &count, diagnostic, &warnings, false);
    if (status != UMC_OK) return status;

    /* Publication begins only after complete grammar and bytecode admission.
     * Location calculation cannot refuse a previously admitted byte span. */
    *report = (UmcSourceReport){0};
    report->instruction_count = count;
    for (size_t i = 0U; i < warnings.count; ++i) {
        if (warnings.read[i]) continue;
        UmcSourceWarning warning = warnings.declarations[i];
        UmcDiagnostic location = {0};
        location.source_offset = warning.source_offset;
        umc_locate_diagnostic(source, length, &location);
        warning.line = location.line;
        warning.column = location.column;
        report->warnings[report->warning_count++] = warning;
    }
    return UMC_OK;
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
        /* Standalone updates reuse runtime arithmetic; retain the earlier source-only mask for review. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS |
            UMC_FEATURE_ELSE_IF_CHAINS | UMC_FEATURE_REMAINDER | UMC_FEATURE_BITWISE_COMPLEMENT |
            UMC_FEATURE_BINARY_BITWISE;
#endif
/* Preserve the earlier feature declaration for review. The lexer now admits separated digits; advertise this source-only addition without changing runtime or wire requirements. The former implementation is retained for owner review. */
#if 0
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS |
            UMC_FEATURE_ELSE_IF_CHAINS | UMC_FEATURE_REMAINDER | UMC_FEATURE_BITWISE_COMPLEMENT |
            UMC_FEATURE_BINARY_BITWISE | UMC_FEATURE_INCREMENT_STATEMENTS;
#endif
        return UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON | UMC_FEATURE_LOGICAL_NOT |
            UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS | UMC_FEATURE_LOCAL_INTEGERS | UMC_FEATURE_LOGICAL_CONNECTIVES | UMC_FEATURE_CONDITIONAL_EXPRESSIONS |
            UMC_FEATURE_COMPOUND_ASSIGNMENTS | UMC_FEATURE_IF_STATEMENTS |
            UMC_FEATURE_ELSE_IF_CHAINS | UMC_FEATURE_REMAINDER | UMC_FEATURE_BITWISE_COMPLEMENT |
/* Report source-level write rejection without attributing const semantics to the bytecode runtime. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_BINARY_BITWISE | UMC_FEATURE_INCREMENT_STATEMENTS | UMC_FEATURE_DIGIT_SEPARATORS;
#endif
            UMC_FEATURE_BINARY_BITWISE | UMC_FEATURE_INCREMENT_STATEMENTS | UMC_FEATURE_DIGIT_SEPARATORS |
/* Advertise branch-local source scope separately from unchanged runtime slot and wire capabilities. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_READONLY_LOCALS;
#endif
/* Report standalone compound statements as a compiler grammar capability; existing runtime instructions and wire formats remain unchanged. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_READONLY_LOCALS | UMC_FEATURE_BRANCH_LOCALS;
#endif
/* Expose the optional source diagnostic service without changing the runtime or bytecode capability masks. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_READONLY_LOCALS | UMC_FEATURE_BRANCH_LOCALS | UMC_FEATURE_COMPOUND_STATEMENTS;
#endif
            UMC_FEATURE_READONLY_LOCALS | UMC_FEATURE_BRANCH_LOCALS | UMC_FEATURE_COMPOUND_STATEMENTS |
/* Advertise checked shifts in the compiler that admits their source spelling. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_SOURCE_WARNINGS;
#endif
/* Advertise empty source statements only from the compiler; they emit no new runtime operation. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_SOURCE_WARNINGS | UMC_FEATURE_CHECKED_SHIFTS;
#endif
/* ASCII character constants add only source spelling; preserve the previous capability expression for review. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_SOURCE_WARNINGS | UMC_FEATURE_CHECKED_SHIFTS | UMC_FEATURE_EMPTY_STATEMENTS;
#endif
            UMC_FEATURE_SOURCE_WARNINGS | UMC_FEATURE_CHECKED_SHIFTS | UMC_FEATURE_EMPTY_STATEMENTS |
/* Add the source-only declaration-list capability while preserving the previous linked compiler mask for review. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_ASCII_CHARACTER_LITERALS;
#endif
/* Advertise Boolean spelling only from the compiler; existing integer runtime and wire behavior remain unchanged. The former implementation is retained for owner review. */
#if 0
            UMC_FEATURE_ASCII_CHARACTER_LITERALS | UMC_FEATURE_DECLARATOR_LISTS;
#endif
            UMC_FEATURE_ASCII_CHARACTER_LITERALS | UMC_FEATURE_DECLARATOR_LISTS |
            UMC_FEATURE_BOOLEAN_LITERALS;
    case UMC_PROFILE_LOCAL_VALUES: return UMC_LOCAL_LIMIT;
    case UMC_PROFILE_IDENTIFIER_BYTES: return UMC_IDENTIFIER_LIMIT;
    case UMC_PROFILE_SOURCE_BYTES: return UMC_SOURCE_LIMIT;
    case UMC_PROFILE_PROGRAM_INSTRUCTIONS: return UMC_PROGRAM_LIMIT;
    case UMC_PROFILE_PARSE_DEPTH: return UMC_PARSE_DEPTH_LIMIT;
    default: return 0U;
    }
}
