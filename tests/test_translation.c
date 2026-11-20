/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_translation.c
 *
 * PURPOSE:
 *   Exercise the bounded C and Java translation profile in both directions.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "umicom_c/translate.h"
#include <stdio.h>
#include <string.h>

#define CHECK_TRANSLATION(condition) do { \
    if (!(condition)) { fprintf(stderr, "%d: %s\n", __LINE__, #condition); return 1; } \
} while (0)

static int roundtrip(void)
{
    const char c[] = " int answer (void) { return (10 + 4) * 3; } ";
    char java[UMC_TRANSLATION_OUTPUT_CAPACITY];
    char translated[UMC_TRANSLATION_OUTPUT_CAPACITY];
    size_t java_size;
    size_t c_size;
    UmcDiagnostic diagnostic;
    CHECK_TRANSLATION(umc_translate_integer_answer(UMC_TRANSLATE_C_TO_JAVA, c,
        sizeof c - 1U, java, sizeof java, &java_size, &diagnostic) == UMC_OK);
    CHECK_TRANSLATION(java_size == strlen(java) && diagnostic.status == UMC_OK);
    CHECK_TRANSLATION(strstr(java, "public final class UmicomAnswer") != NULL);
    CHECK_TRANSLATION(strstr(java, "return (10 + 4) * 3;") != NULL);
    CHECK_TRANSLATION(umc_translate_integer_answer(UMC_TRANSLATE_JAVA_TO_C, java,
        java_size, translated, sizeof translated, &c_size, &diagnostic) == UMC_OK);
    CHECK_TRANSLATION(c_size == strlen(translated) && diagnostic.status == UMC_OK);
    CHECK_TRANSLATION(strcmp(translated, "int answer(void) {\n    return (10 + 4) * 3;\n}\n") == 0);
    return 0;
}

static int refusal(const char *case_name)
{
    static const struct {
        const char *name;
        const char *source;
        UmcStatus expected;
    } cases[] = {
        {"empty", "int answer(void){return ;}", UMC_SYNTAX_ERROR},
        {"extra", "int answer(void){return 41; return 42;}", UMC_SYNTAX_ERROR},
        {"trailing", "int answer(void){return 42;} extra", UMC_SYNTAX_ERROR},
        {"octal", "int answer(void){return 042;}", UMC_SYNTAX_ERROR},
        {"literal", "int answer(void){return 2147483648;}", UMC_SYNTAX_ERROR},
        {"comment", "int answer(void){return 6 /* value */ * 7;}", UMC_SYNTAX_ERROR},
        {"operator", "int answer(void){return 6--7;}", UMC_SYNTAX_ERROR},
        {"overflow", "int answer(void){return 2147483647+1;}", UMC_ARITHMETIC_ERROR},
        {"division", "int answer(void){\nreturn 42/(7-7);}", UMC_ARITHMETIC_ERROR},
        {"capacity", "int answer(void){return 42;}", UMC_BUFFER_TOO_SMALL},
        {"java-shape", "public class UmicomAnswer { public static int answer(){return 42;} }",
            UMC_SYNTAX_ERROR}
    };
    size_t index = 0U;
    while (index < sizeof cases / sizeof cases[0] && strcmp(case_name, cases[index].name) != 0) ++index;
    if (index == sizeof cases / sizeof cases[0]) return 2;
    char output[UMC_TRANSLATION_OUTPUT_CAPACITY] = "previous output";
    char original[sizeof output];
    memcpy(original, output, sizeof original);
    size_t out_size = 91U;
    UmcDiagnostic diagnostic;
    const UmcTranslationDirection direction = !strcmp(case_name, "java-shape")
        ? UMC_TRANSLATE_JAVA_TO_C : UMC_TRANSLATE_C_TO_JAVA;
    const size_t capacity = !strcmp(case_name, "capacity") ? 3U : sizeof output;
    const size_t source_length = strlen(cases[index].source);
    CHECK_TRANSLATION(umc_translate_integer_answer(direction, cases[index].source, source_length,
        output, capacity, &out_size, &diagnostic) == cases[index].expected);
    CHECK_TRANSLATION(memcmp(output, original, sizeof output) == 0 && out_size == 91U);
    CHECK_TRANSLATION(diagnostic.status == cases[index].expected &&
        diagnostic.source_offset <= source_length && diagnostic.message != NULL);
    if (!strcmp(case_name, "division")) CHECK_TRANSLATION(diagnostic.line == 2U);
    return 0;
}

static int function_roundtrip(void)
{
    const char primary[] = "int answer(void) {\n int net = 6;\n int fee = 3;\n"
        " net = (net + fee) * 5;\n return net - fee;\n}\n";
    const char expected[] = "int answer(void) {\n    int net = 6;\n    int fee = 3;\n"
        "    net = (net + fee) * 5;\n    return net - fee;\n}\n";
    char java[UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY];
    char c[UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY];
    size_t java_size = 0U, c_size = 0U;
    UmcDiagnostic diagnostic;
    CHECK_TRANSLATION(umc_translate_integer_function(UMC_TRANSLATE_C_TO_JAVA, primary,
        sizeof primary - 1U, java, sizeof java, &java_size, &diagnostic) == UMC_OK);
    CHECK_TRANSLATION(java_size == strlen(java) && strstr(java, "        net = (net + fee) * 5;\n") != NULL);
    CHECK_TRANSLATION(umc_translate_integer_function(UMC_TRANSLATE_JAVA_TO_C, java,
        java_size, c, sizeof c, &c_size, &diagnostic) == UMC_OK);
    CHECK_TRANSLATION(c_size == strlen(c) && strcmp(c, expected) == 0);
    CHECK_TRANSLATION(umc_translate_integer_answer(UMC_TRANSLATE_C_TO_JAVA, primary,
        sizeof primary - 1U, java, sizeof java, &java_size, &diagnostic) == UMC_SYNTAX_ERROR);
    return 0;
}

static int function_refusal(const char *case_name)
{
    static const struct {
        const char *name;
        UmcTranslationDirection direction;
        const char *source;
        UmcStatus expected;
    } cases[] = {
        {"function-uninitialized", UMC_TRANSLATE_C_TO_JAVA,
            "int answer(void) { int net; return net; }", UMC_SYNTAX_ERROR},
        {"function-undeclared", UMC_TRANSLATE_C_TO_JAVA,
            "int answer(void) { int net=6;\n return fee+net; }", UMC_SYNTAX_ERROR},
        {"function-reserved-java", UMC_TRANSLATE_C_TO_JAVA,
            "int answer(void) { int class = 42; return class; }", UMC_SYNTAX_ERROR},
        {"function-reserved-c", UMC_TRANSLATE_JAVA_TO_C,
            "public final class UmicomAnswer { public static int answer() { int restrict=42; return restrict; }}",
            UMC_SYNTAX_ERROR},
        {"function-literal", UMC_TRANSLATE_C_TO_JAVA,
            "int answer(void) { int net=0xff; return net; }", UMC_SYNTAX_ERROR},
        {"function-increment", UMC_TRANSLATE_C_TO_JAVA,
            "int answer(void) { int net=41; net++; return net; }", UMC_SYNTAX_ERROR},
        {"function-overflow", UMC_TRANSLATE_C_TO_JAVA,
            "int answer(void) { int net=2147483647; net=net+1; return 42; }", UMC_ARITHMETIC_ERROR},
        {"function-capacity", UMC_TRANSLATE_C_TO_JAVA,
            "int answer(void) { int net=42; return net; }", UMC_BUFFER_TOO_SMALL}
    };
    for (size_t i = 0U; i < sizeof cases / sizeof cases[0]; ++i) {
        if (strcmp(case_name, cases[i].name) != 0) continue;
        char output[UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY] = "prior";
        const size_t capacity = cases[i].expected == UMC_BUFFER_TOO_SMALL ? 4U : sizeof output;
        size_t used = 810U;
        UmcDiagnostic diagnostic;
        CHECK_TRANSLATION(umc_translate_integer_function(cases[i].direction, cases[i].source,
            strlen(cases[i].source), output, capacity, &used, &diagnostic) == cases[i].expected);
        CHECK_TRANSLATION(used == 810U && strcmp(output, "prior") == 0);
        CHECK_TRANSLATION(diagnostic.status == cases[i].expected && diagnostic.message != NULL);
        if (strcmp(case_name, "function-undeclared") == 0) CHECK_TRANSLATION(diagnostic.line == 2U);
        return 0;
    }
    return 2;
}

static int function_statement_limit(void)
{
    char source[UMC_TRANSLATION_FUNCTION_LIMIT] = "int answer(void) { int net=0; ";
    const char assignment[] = "net=net+1; ";
    char *cursor = source + strlen(source);
    for (size_t i = 0U; i < UMC_TRANSLATION_STATEMENT_LIMIT - 2U; ++i) {
        memcpy(cursor, assignment, sizeof assignment - 1U);
        cursor += sizeof assignment - 1U;
    }
    const char finish[] = "return net; }";
    memcpy(cursor, finish, sizeof finish);
    char output[UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY];
    size_t used = 0U;
    CHECK_TRANSLATION(umc_translate_integer_function(UMC_TRANSLATE_C_TO_JAVA, source,
        strlen(source), output, sizeof output, &used, NULL) == UMC_OK);
    const size_t previous = used;
    memcpy(cursor, assignment, sizeof assignment - 1U);
    memcpy(cursor + sizeof assignment - 1U, finish, sizeof finish);
    CHECK_TRANSLATION(umc_translate_integer_function(UMC_TRANSLATE_C_TO_JAVA, source,
        strlen(source), output, sizeof output, &used, NULL) == UMC_PROGRAM_TOO_LARGE);
    CHECK_TRANSLATION(used == previous && strlen(output) == previous);
    return 0;
}

static int function_comment_roundtrip(bool locals)
{
    /* Delimiters inside both forms of comment are body text. Keep the exact
     * body bytes, including CRLF, across both generated envelopes. */
    const char simple_body[] = "\n    return 6 /* keep the factor; } */ * 7;\n";
    const char local_body[] = "\r\n    /* Initialize; } */ int net = 20;\r\n"
        "    net = net + 1; // Keep the adjustment {; }\r\n    return net * 2;\r\n";
    const char *body = locals ? local_body : simple_body;
    char original[512];
    const int written = snprintf(original, sizeof original, "int answer(void) {%s}\n", body);
    CHECK_TRANSLATION(written > 0 && (size_t)written < sizeof original);
    char java[UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY], c[UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY];
    size_t java_size = 0U, c_size = 0U;
    UmcDiagnostic diagnostic;
    CHECK_TRANSLATION(umc_translate_integer_function(UMC_TRANSLATE_C_TO_JAVA, original,
        (size_t)written, java, sizeof java, &java_size, &diagnostic) == UMC_OK);
    CHECK_TRANSLATION(strstr(java, body) != NULL && java_size == strlen(java));
    CHECK_TRANSLATION(umc_translate_integer_function(UMC_TRANSLATE_JAVA_TO_C, java,
        java_size, c, sizeof c, &c_size, &diagnostic) == UMC_OK);
    CHECK_TRANSLATION(c_size == (size_t)written && strcmp(c, original) == 0);
    return 0;
}

static int function_comment_refusal(const char *which)
{
    /* Keep diagnostic positions in the original text and preserve every
     * output byte when a body comment cannot safely cross two lexers. */
    static const struct {
        const char *name, *source, *offset_at;
        UmcStatus status;
    } cases[] = {
        {"function-comment-open", "int answer(void) {\n /* unfinished", "/*", UMC_SYNTAX_ERROR},
        {"function-comment-unicode", "int answer(void) {\n /* \\u000a */ return 42; }", "\\u", UMC_SYNTAX_ERROR},
        {"function-comment-splice", "int answer(void) {\n // runs on \\\r\n return 42; }", "\\", UMC_SYNTAX_ERROR},
        {"function-comment-outside", "/* leading */ int answer(void){return 42;}", "/*", UMC_SYNTAX_ERROR},
        {"function-comment-trailing", "int answer(void){return 42; /* local */ } /* trailing */", "/* trailing", UMC_SYNTAX_ERROR},
        {"function-comment-capacity", "int answer(void){return 42; /* bounded */ }", NULL, UMC_BUFFER_TOO_SMALL},
        {"function-comment-arithmetic", "int answer(void){\n /* keep; } */\n return 4/0;\n}", "/0", UMC_ARITHMETIC_ERROR}
    };
    for (size_t i = 0U; i < sizeof cases / sizeof cases[0]; ++i) {
        if (strcmp(which, cases[i].name) != 0) continue;
        char output[128] = "unchanged";
        char expected[sizeof output];
        memcpy(expected, output, sizeof expected);
        size_t used = 901U;
        UmcDiagnostic diagnostic;
        const size_t capacity = cases[i].status == UMC_BUFFER_TOO_SMALL ? 4U : sizeof output;
        CHECK_TRANSLATION(umc_translate_integer_function(UMC_TRANSLATE_C_TO_JAVA, cases[i].source,
            strlen(cases[i].source), output, capacity, &used, &diagnostic) == cases[i].status);
        CHECK_TRANSLATION(used == 901U && memcmp(output, expected, sizeof output) == 0);
        CHECK_TRANSLATION(diagnostic.status == cases[i].status);
        if (cases[i].offset_at != NULL) {
            const char *offset = strstr(cases[i].source, cases[i].offset_at);
            CHECK_TRANSLATION(offset != NULL && diagnostic.source_offset == (size_t)(offset - cases[i].source));
        }
        return 0;
    }
    return 2;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (!strcmp(argv[1], "roundtrip")) return roundtrip();
    if (!strcmp(argv[1], "function-roundtrip")) return function_roundtrip();
    if (!strcmp(argv[1], "function-statement-limit")) return function_statement_limit();
    if (!strcmp(argv[1], "function-comment-expression")) return function_comment_roundtrip(false);
    if (!strcmp(argv[1], "function-comment-locals")) return function_comment_roundtrip(true);
    if (strncmp(argv[1], "function-comment-", sizeof "function-comment-" - 1U) == 0)
        return function_comment_refusal(argv[1]);
    if (strncmp(argv[1], "function-", sizeof "function-" - 1U) == 0) return function_refusal(argv[1]);
    return refusal(argv[1]);
}
