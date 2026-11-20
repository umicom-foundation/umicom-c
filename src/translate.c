/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/translate.c
 *
 * PURPOSE:
 *   Reuse the checked integer compiler to admit C and Java translation.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "umicom_c/translate.h"
#include "internal.h"
#include <stdbool.h>
#include <string.h>

typedef struct SourceCursor {
    const char *text;
    size_t size;
    size_t offset;
} SourceCursor;

static bool space(char ch)
{
    return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

static bool digit(char ch)
{
    return ch >= '0' && ch <= '9';
}

static bool identifier_char(char ch)
{
    return digit(ch) || ch == '_' || (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z');
}

static void skip_space(SourceCursor *source)
{
    while (source->offset < source->size && space(source->text[source->offset])) ++source->offset;
}

static bool take(SourceCursor *source, const char *token)
{
    skip_space(source);
    const size_t length = strlen(token);
    if (length > source->size - source->offset ||
        memcmp(source->text + source->offset, token, length) != 0)
        return false;
    const size_t end = source->offset + length;
    if (identifier_char(token[0]) && end < source->size && identifier_char(source->text[end]))
        return false;
    source->offset = end;
    return true;
}

static UmcStatus refuse(const char *source, size_t size, size_t offset,
    UmcStatus status, const char *message, UmcDiagnostic *diagnostic)
{
    if (diagnostic != NULL) {
        *diagnostic = (UmcDiagnostic){.status = status, .source_offset = offset, .message = message};
        if (source != NULL && size <= UMC_SOURCE_LIMIT)
            umc_locate_diagnostic(source, size, diagnostic);
    }
    return status;
}

/* Share the function envelope. The earlier expression entry still requires
 * return immediately, while the extended entry checks each local statement. */
static bool source_header(SourceCursor *source, UmcTranslationDirection direction, bool require_return)
{
    if (direction == UMC_TRANSLATE_JAVA_TO_C &&
        !(take(source, "public") && take(source, "final") && take(source, "class") &&
          take(source, "UmicomAnswer") && take(source, "{") && take(source, "public") &&
          take(source, "static")))
        return false;
    return take(source, "int") && take(source, "answer") && take(source, "(") &&
        (direction == UMC_TRANSLATE_JAVA_TO_C || take(source, "void")) &&
        take(source, ")") && take(source, "{") && (!require_return || take(source, "return"));
}

/* Restrict source to spellings that the two real languages interpret alike.
 * The canonical compiler still owns expression grammar, precedence and
 * arithmetic. This pass only excludes dialect spellings and extra tokens. */
static bool expression_text(const char *text, size_t size, size_t *bad_offset)
{
    for (size_t i = 0U; i < size; ++i) {
        const char ch = text[i];
        if (digit(ch)) {
            if (ch == '0' && i + 1U < size && digit(text[i + 1U])) {
                *bad_offset = i;
                return false;
            }
            uint32_t value = (uint32_t)(ch - '0');
            while (i + 1U < size && digit(text[i + 1U])) {
                const uint32_t next = (uint32_t)(text[++i] - '0');
                if (value > (UINT32_C(2147483647) - next) / 10U) {
                    *bad_offset = i;
                    return false;
                }
                value = value * 10U + next;
            }
            continue;
        }
        if (space(ch) || ch == '(' || ch == ')' || ch == '%' || ch == '*') continue;
        if ((ch == '+' || ch == '-') && (i + 1U == size || text[i + 1U] != ch)) continue;
        if (ch == '/' && (i + 1U == size || (text[i + 1U] != '/' && text[i + 1U] != '*'))) continue;
        *bad_offset = i;
        return false;
    }
    return true;
}

UmcStatus umc_translate_integer_answer(UmcTranslationDirection direction, const char *source,
    size_t source_size, char *output, size_t output_capacity, size_t *output_size,
    UmcDiagnostic *diagnostic)
{
    static const char native_prefix[] = "int main(void){return ";
    static const char native_suffix[] = ";}";
    static const char java_prefix[] = "public final class UmicomAnswer {\n    public static int answer() {\n        return ";
    static const char java_suffix[] = ";\n    }\n}\n";
    static const char c_prefix[] = "int answer(void) {\n    return ";
    static const char c_suffix[] = ";\n}\n";
    if (source == NULL || output == NULL || output_size == NULL ||
        (direction != UMC_TRANSLATE_C_TO_JAVA && direction != UMC_TRANSLATE_JAVA_TO_C))
        return refuse(source, source_size, 0U, UMC_INVALID_ARGUMENT, "invalid translation request", diagnostic);
    if (source_size > UMC_SOURCE_LIMIT)
        return refuse(source, source_size, 0U, UMC_SOURCE_TOO_LARGE, "source exceeds the translation limit", diagnostic);

    SourceCursor cursor = {source, source_size, 0U};
    if (!source_header(&cursor, direction, true))
        return refuse(source, source_size, cursor.offset, UMC_SYNTAX_ERROR, "unsupported function declaration", diagnostic);
    skip_space(&cursor);
    const size_t start = cursor.offset;
    while (cursor.offset < cursor.size && cursor.text[cursor.offset] != ';') ++cursor.offset;
    size_t end = cursor.offset;
    while (end > start && space(source[end - 1U])) --end;
    const size_t expression_size = end - start;
    if (expression_size > UMC_TRANSLATION_EXPRESSION_LIMIT)
        return refuse(source, source_size, start, UMC_SOURCE_TOO_LARGE, "expression exceeds the translation limit", diagnostic);
    if (expression_size == 0U || !take(&cursor, ";") || !take(&cursor, "}") ||
        (direction == UMC_TRANSLATE_JAVA_TO_C && !take(&cursor, "}")))
        return refuse(source, source_size, cursor.offset, UMC_SYNTAX_ERROR, "expected one complete return expression", diagnostic);
    skip_space(&cursor);
    if (cursor.offset != cursor.size)
        return refuse(source, source_size, cursor.offset, UMC_SYNTAX_ERROR, "unexpected text after the function", diagnostic);
    size_t bad_offset = 0U;
    if (!expression_text(source + start, expression_size, &bad_offset))
        return refuse(source, source_size, start + bad_offset, UMC_SYNTAX_ERROR, "unsupported arithmetic spelling", diagnostic);

    /* Reuse the compiler and checked execution to establish portable integer
     * semantics before emitting either target. No native tool is launched. */
    const size_t native_prefix_size = sizeof native_prefix - 1U;
    char native[sizeof native_prefix + UMC_TRANSLATION_EXPRESSION_LIMIT + sizeof native_suffix];
    memcpy(native, native_prefix, native_prefix_size);
    memcpy(native + native_prefix_size, source + start, expression_size);
    memcpy(native + native_prefix_size + expression_size, native_suffix, sizeof native_suffix);
    UmcInstruction program[UMC_PROGRAM_LIMIT];
    UmcDiagnostic checked = {0};
    size_t count = 0U;
    UmcStatus status = umc_compile(native, native_prefix_size + expression_size + sizeof native_suffix - 1U,
        program, UMC_PROGRAM_LIMIT, &count, &checked);
    UmcResult result;
    if (status == UMC_OK) status = umc_execute(program, count, UMC_PROGRAM_LIMIT, &result, &checked);
    if (status != UMC_OK) {
        size_t relative = checked.source_offset >= native_prefix_size
            ? checked.source_offset - native_prefix_size : 0U;
        if (relative > expression_size) relative = expression_size;
        return refuse(source, source_size, start + relative, status, checked.message, diagnostic);
    }

    const char *prefix = direction == UMC_TRANSLATE_C_TO_JAVA ? java_prefix : c_prefix;
    const char *suffix = direction == UMC_TRANSLATE_C_TO_JAVA ? java_suffix : c_suffix;
    const size_t prefix_size = strlen(prefix);
    const size_t length = prefix_size + expression_size + strlen(suffix);
    if (output_capacity <= length)
        return refuse(source, source_size, 0U, UMC_BUFFER_TOO_SMALL, "translation output is too small", diagnostic);
    memcpy(output, prefix, prefix_size);
    memcpy(output + prefix_size, source + start, expression_size);
    memcpy(output + prefix_size + expression_size, suffix, strlen(suffix) + 1U);
    *output_size = length;
    if (diagnostic != NULL) *diagnostic = (UmcDiagnostic){.status = UMC_OK};
    return UMC_OK;
}

typedef struct SourceSpan {
    size_t start;
    size_t end;
} SourceSpan;

static bool name_start(char ch)
{
    return identifier_char(ch) && !digit(ch) && ch != '_';
}

static bool portable_name(const char *name, size_t length)
{
    /* Leave words with language or contextual meaning outside the common
     * profile. A target compiler may give them a different interpretation. */
    static const char *const reserved[] = {
        "abstract", "alignas", "alignof", "asm", "assert", "auto", "bool", "boolean",
        "break", "byte", "case", "catch", "char", "class", "const", "constexpr",
        "continue", "default", "do", "double", "else", "enum", "exports", "extends",
        "extern", "false", "final", "finally", "float", "for", "goto", "if",
        "implements", "import", "inline", "instanceof", "int", "interface", "long",
        "module", "native", "new", "null", "nullptr", "open", "opens", "package",
        "permits", "private", "protected", "provides", "public", "record", "register",
        "requires", "restrict", "return", "sealed", "short", "signed", "sizeof",
        "static", "static_assert", "strictfp", "struct", "super", "switch",
        "synchronized", "this", "thread_local", "throw", "throws", "to", "transient",
        "transitive", "true", "try", "typedef", "typeof", "typeof_unqual", "union",
        "unsigned", "uses", "var", "void", "volatile", "when", "while", "with", "yield"
    };
    if (length == 0U || !name_start(name[0])) return false;
    for (size_t i = 0U; i < sizeof reserved / sizeof reserved[0]; ++i)
        if (strlen(reserved[i]) == length && memcmp(name, reserved[i], length) == 0) return false;
    return true;
}

static bool take_local_name(SourceCursor *cursor)
{
    skip_space(cursor);
    const size_t start = cursor->offset;
    while (cursor->offset < cursor->size && identifier_char(cursor->text[cursor->offset]))
        ++cursor->offset;
    return portable_name(cursor->text + start, cursor->offset - start);
}

static bool local_expression_text(const char *text, size_t size, size_t *bad_offset)
{
    /* Preserve the expression-only lexer rules between names. The shared
     * compiler still recognizes complete tokens, assignment order and use. */
    size_t cursor = 0U;
    while (cursor < size) {
        const size_t start = cursor;
        if (name_start(text[cursor]) || text[cursor] == '_') {
            while (cursor < size && identifier_char(text[cursor])) ++cursor;
            if ((start != 0U && digit(text[start - 1U])) || !portable_name(text + start, cursor - start)) {
                *bad_offset = start;
                return false;
            }
        } else {
            while (cursor < size && !name_start(text[cursor]) && text[cursor] != '_') ++cursor;
            size_t relative = 0U;
            if (!expression_text(text + start, cursor - start, &relative)) {
                *bad_offset = start + relative;
                return false;
            }
        }
    }
    return true;
}

/* The original function translator remains the common admission and execution
 * path. The public entry below can now give it a checked comment-free view. */
static UmcStatus translate_uncommented_function(UmcTranslationDirection direction, const char *source,
    size_t source_size, char *output, size_t output_capacity, size_t *output_size,
    UmcDiagnostic *diagnostic)
{
    if (source == NULL || output == NULL || output_size == NULL ||
        (direction != UMC_TRANSLATE_C_TO_JAVA && direction != UMC_TRANSLATE_JAVA_TO_C) ||
        source_size > UMC_SOURCE_LIMIT)
        return umc_translate_integer_answer(direction, source, source_size, output,
            output_capacity, output_size, diagnostic);
    SourceCursor cursor = {source, source_size, 0U};
    if (!source_header(&cursor, direction, false))
        return refuse(source, source_size, cursor.offset, UMC_SYNTAX_ERROR, "unsupported function declaration", diagnostic);
    skip_space(&cursor);
    const size_t body_start = cursor.offset;
    if (take(&cursor, "return"))
        return umc_translate_integer_answer(direction, source, source_size, output,
            output_capacity, output_size, diagnostic);
    if (source_size > UMC_TRANSLATION_FUNCTION_LIMIT)
        return refuse(source, source_size, body_start, UMC_SOURCE_TOO_LARGE, "function exceeds the translation limit", diagnostic);

    SourceSpan statements[UMC_TRANSLATION_STATEMENT_LIMIT];
    size_t count = 0U;
    bool returning = false;
    do {
        skip_space(&cursor);
        const size_t start = cursor.offset;
        if (count == UMC_TRANSLATION_STATEMENT_LIMIT)
            return refuse(source, source_size, start, UMC_PROGRAM_TOO_LARGE, "too many translated statements", diagnostic);
        returning = take(&cursor, "return");
        if (!returning) {
            (void)take(&cursor, "int");
            if (!take_local_name(&cursor) || !take(&cursor, "="))
                return refuse(source, source_size, start, UMC_SYNTAX_ERROR, "expected an initialized local or assignment", diagnostic);
        }
        skip_space(&cursor);
        const size_t expression_start = cursor.offset;
        while (cursor.offset < cursor.size && source[cursor.offset] != ';') ++cursor.offset;
        const size_t length = cursor.offset - expression_start;
        if (length == 0U || !take(&cursor, ";"))
            return refuse(source, source_size, expression_start, UMC_SYNTAX_ERROR, "expected a complete local expression", diagnostic);
        if (length > UMC_TRANSLATION_EXPRESSION_LIMIT)
            return refuse(source, source_size, expression_start, UMC_SOURCE_TOO_LARGE, "expression exceeds the translation limit", diagnostic);
        size_t bad = 0U;
        if (!local_expression_text(source + expression_start, length, &bad))
            return refuse(source, source_size, expression_start + bad, UMC_SYNTAX_ERROR, "unsupported arithmetic spelling or local name", diagnostic);
        statements[count++] = (SourceSpan){start, cursor.offset};
    } while (!returning);

    const size_t body_end = cursor.offset;
    if (!take(&cursor, "}") || (direction == UMC_TRANSLATE_JAVA_TO_C && !take(&cursor, "}")))
        return refuse(source, source_size, cursor.offset, UMC_SYNTAX_ERROR, "expected the end of the translated function", diagnostic);
    skip_space(&cursor);
    if (cursor.offset != cursor.size)
        return refuse(source, source_size, cursor.offset, UMC_SYNTAX_ERROR, "unexpected text after the function", diagnostic);

    const char native_prefix[] = "int main(void){";
    const size_t native_start = sizeof native_prefix - 1U;
    const size_t body_length = body_end - body_start;
    char native[sizeof native_prefix + UMC_TRANSLATION_FUNCTION_LIMIT + 1U];
    memcpy(native, native_prefix, native_start);
    memcpy(native + native_start, source + body_start, body_length);
    native[native_start + body_length] = '}';
    UmcInstruction program[UMC_PROGRAM_LIMIT];
    size_t instruction_count = 0U;
    UmcDiagnostic checked = {0};
    UmcStatus status = umc_compile(native, native_start + body_length + 1U, program,
        UMC_PROGRAM_LIMIT, &instruction_count, &checked);
    UmcResult result;
    if (status == UMC_OK) status = umc_execute(program, instruction_count, UMC_PROGRAM_LIMIT, &result, &checked);
    if (status != UMC_OK) {
        size_t offset = checked.source_offset >= native_start ? checked.source_offset - native_start : 0U;
        if (offset > body_length) offset = body_length;
        return refuse(source, source_size, body_start + offset, status, checked.message, diagnostic);
    }

    /* Only publish after the checked body and all formatted statements fit. */
    const bool java = direction == UMC_TRANSLATE_C_TO_JAVA;
    const char *prefix = java ? "public final class UmicomAnswer {\n    public static int answer() {\n" : "int answer(void) {\n";
    const char *indent = java ? "        " : "    ";
    const char *suffix = java ? "    }\n}\n" : "}\n";
    const size_t indent_size = strlen(indent);
    size_t length = strlen(prefix) + strlen(suffix) + count * (indent_size + 1U);
    for (size_t i = 0U; i < count; ++i) length += statements[i].end - statements[i].start;
    if (output_capacity <= length)
        return refuse(source, source_size, 0U, UMC_BUFFER_TOO_SMALL, "translation output is too small", diagnostic);

    size_t used = strlen(prefix);
    memcpy(output, prefix, used);
    for (size_t i = 0U; i < count; ++i) {
        const size_t statement_size = statements[i].end - statements[i].start;
        memcpy(output + used, indent, indent_size);
        used += indent_size;
        memcpy(output + used, source + statements[i].start, statement_size);
        used += statement_size;
        output[used++] = '\n';
    }
    memcpy(output + used, suffix, strlen(suffix) + 1U);
    *output_size = length;
    if (diagnostic != NULL) *diagnostic = (UmcDiagnostic){.status = UMC_OK};
    return UMC_OK;
}

static void seek_comment(UmcLexer *lexer)
{
    /* The common translator grammar admits no strings or character literals.
     * The existing checked parser still owns refusal of all other tokens. */
    while (lexer->position < lexer->length && !umc_lex_at_comment(lexer)) ++lexer->position;
}

static bool portable_comment_byte(const char *source, size_t size, size_t offset)
{
    const unsigned char ch = (unsigned char)source[offset];
    if ((ch < 32U && !space((char)ch)) || ch > 126U) return false;
    if (ch != '\\') return true;

    /* Java handles Unicode escapes before comments. C removes a line splice
     * before tokenization. Either would alter the preserved body on one host. */
    size_t following = offset + 1U;
    if (following < size && source[following] == 'u') return false;
    while (following < size && (source[following] == ' ' || source[following] == '\t')) ++following;
    return following == size || (source[following] != '\n' && source[following] != '\r');
}

static UmcStatus translate_commented_function(UmcTranslationDirection direction, UmcLexer lexer,
    char *output, size_t output_capacity, size_t *output_size, UmcDiagnostic *diagnostic)
{
    const char *source = lexer.source;
    const size_t source_size = lexer.length;
    const size_t first_comment = lexer.position;
    if (source_size > UMC_TRANSLATION_FUNCTION_LIMIT)
        return refuse(source, source_size, first_comment, UMC_SOURCE_TOO_LARGE,
            "commented function exceeds the translation limit", diagnostic);

    /* Mask only in a private view; keeping the same byte count and line ends
     * means compiler locations map directly back to the untouched input. */
    char normalized[UMC_TRANSLATION_FUNCTION_LIMIT];
    memcpy(normalized, source, source_size);
    size_t comment_start = first_comment, comment_end = first_comment;
    while (lexer.position < source_size) {
        comment_start = lexer.position;
        UmcDiagnostic comment = {0};
        UmcStatus status = umc_lex_skip_comment(&lexer, &comment);
        if (status != UMC_OK)
            return refuse(source, source_size, comment.source_offset, status,
                "unterminated or invalid function comment", diagnostic);
        comment_end = lexer.position;
        for (size_t at = comment_start; at < comment_end; ++at) {
            if (!portable_comment_byte(source, source_size, at))
                return refuse(source, source_size, at, UMC_SYNTAX_ERROR,
                    "comment contains text outside the common C and Java profile", diagnostic);
            if (source[at] != '\n' && source[at] != '\r') normalized[at] = ' ';
        }
        seek_comment(&lexer);
    }

    SourceCursor body = {normalized, source_size, 0U};
    if (!source_header(&body, direction, false))
        return refuse(source, source_size, body.offset, UMC_SYNTAX_ERROR,
            "unsupported function declaration", diagnostic);
    const char *end = memchr(normalized + body.offset, '}', source_size - body.offset);
    if (end == NULL)
        return refuse(source, source_size, source_size, UMC_SYNTAX_ERROR,
            "expected the end of the translated function", diagnostic);
    const size_t end_offset = (size_t)(end - normalized);
    if (first_comment < body.offset || comment_end > end_offset)
        return refuse(source, source_size, first_comment < body.offset ? first_comment : comment_start,
            UMC_SYNTAX_ERROR, "comments are supported only inside the translated function body", diagnostic);

    /* Let the same bounded compiler and VM validate the complete program.
     * Temporary output keeps the user's destination untouched on refusal. */
    char checked_output[UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY];
    size_t checked_size = 0U;
    UmcDiagnostic checked = {0};
    UmcStatus status = translate_uncommented_function(direction, normalized, source_size,
        checked_output, sizeof checked_output, &checked_size, &checked);
    if (status != UMC_OK)
        return refuse(source, source_size, checked.source_offset, status, checked.message, diagnostic);

    /* Replace only the language envelope. The already checked body is copied
     * with its exact comments and line endings after capacity admission. */
    const bool java = direction == UMC_TRANSLATE_C_TO_JAVA;
    const char *prefix = java ? "public final class UmicomAnswer {\n    public static int answer() {" : "int answer(void) {";
    const char *suffix = java ? "}\n}\n" : "}\n";
    const size_t prefix_size = strlen(prefix);
    const size_t body_size = end_offset - body.offset;
    const size_t length = prefix_size + body_size + strlen(suffix);
    if (output_capacity <= length)
        return refuse(source, source_size, 0U, UMC_BUFFER_TOO_SMALL, "translation output is too small", diagnostic);
    memcpy(output, prefix, prefix_size);
    memcpy(output + prefix_size, source + body.offset, body_size);
    memcpy(output + prefix_size + body_size, suffix, strlen(suffix) + 1U);
    *output_size = length;
    if (diagnostic != NULL) *diagnostic = (UmcDiagnostic){.status = UMC_OK};
    return UMC_OK;
}

UmcStatus umc_translate_integer_function(UmcTranslationDirection direction, const char *source,
    size_t source_size, char *output, size_t output_capacity, size_t *output_size,
    UmcDiagnostic *diagnostic)
{
    /* Preserve the existing argument rules, output and limits when the source
     * has no comment. Only the checked function entry grows this profile. */
    if (source != NULL && output != NULL && output_size != NULL && source_size <= UMC_SOURCE_LIMIT &&
            (direction == UMC_TRANSLATE_C_TO_JAVA || direction == UMC_TRANSLATE_JAVA_TO_C)) {
        UmcLexer lexer = {source, source_size, 0U};
        seek_comment(&lexer);
        if (lexer.position < source_size)
            return translate_commented_function(direction, lexer, output, output_capacity, output_size, diagnostic);
    }
    return translate_uncommented_function(direction, source, source_size, output,
        output_capacity, output_size, diagnostic);
}
