/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_integer_literals.c
 *
 * PURPOSE:
 *   Verify numeric spelling, refusal locations, borrowed spans and unchanged wire meaning.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
#include <limits.h>

static int evaluate(const char *text, int32_t expected)
{
    char source[256];
    int length = snprintf(source, sizeof source, "int main(void){return %s;}", text);
    CHECK(length > 0 && (size_t)length < sizeof source);
    UmcInstruction program[UMC_PROGRAM_LIMIT], restored[UMC_PROGRAM_LIMIT];
    uint8_t bytes[UMC_BYTECODE_LIMIT];
    UmcDiagnostic diagnostic; UmcResult result;
    size_t count = 0U, byte_count = 0U, restored_count = 0U;
    CHECK(umc_compile(source, (size_t)length, program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    CHECK(umc_execute(program, count, count, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == expected);
    CHECK(umc_encode(program, count, bytes, sizeof bytes, &byte_count, &diagnostic) == UMC_OK);
    CHECK(umc_decode(bytes, byte_count, restored, UMC_PROGRAM_LIMIT, &restored_count, &diagnostic) == UMC_OK);
    CHECK(restored_count == count);
    CHECK(umc_execute(restored, restored_count, restored_count, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == expected);
    return 0;
}

static int values(void)
{
    static const struct { const char *source; int32_t result; } cases[] = {
        {"0x0", 0}, {"0X2A", 42}, {"0x2a", 42}, {"0xaBcD", 43981},
        {"0x7fffffff", INT32_MAX}, {"0X7FFFFFFF", INT32_MAX},
        {"-0x7fffffff-1", INT32_MIN}, {"0x0000000000002a", 42},
        {"0b0", 0}, {"0B1", 1}, {"0b101010", 42},
        {"0b00000000000000000000000000000000000000000001", 1},
        {"0b1111111111111111111111111111111", INT32_MAX},
        {"-0b1111111111111111111111111111111-1", INT32_MIN},
        {"0x1e+2", 32}, {"0x1e-2", 28}, {"0b10+0xA*3", 32},
        {"(0X10-0B10)/0x7", 2}, {"!0b0", 1}, {"!!0X10", 1},
        {"0x2a == 0b101010", 1}, {"0x2A < 0b101010", 0},
        {"-0xA/0b11", -3}, {"0x2/* separate */+0B11", 5}
    };
    for (size_t i = 0U; i < sizeof cases / sizeof cases[0]; ++i)
        CHECK(evaluate(cases[i].source, cases[i].result) == 0);
    return 0;
}

static int refusals(void)
{
    static const struct { const char *text; UmcStatus status; size_t offset; } cases[] = {
        {"0x", UMC_SYNTAX_ERROR, 0U}, {"0X", UMC_SYNTAX_ERROR, 0U},
        {"0b", UMC_SYNTAX_ERROR, 0U}, {"0B", UMC_SYNTAX_ERROR, 0U},
        {"0xg", UMC_SYNTAX_ERROR, 0U}, {"0b2", UMC_SYNTAX_ERROR, 2U},
        {"0b102", UMC_SYNTAX_ERROR, 4U}, {"0b10e2", UMC_SYNTAX_ERROR, 4U},
        {"0x1g", UMC_SYNTAX_ERROR, 3U}, {"0x1u", UMC_SYNTAX_ERROR, 3U},
        {"0x1p0", UMC_SYNTAX_ERROR, 3U}, {"0x1.0", UMC_SYNTAX_ERROR, 3U},
        {"0x1'2", UMC_SYNTAX_ERROR, 3U}, {"0b1_0", UMC_SYNTAX_ERROR, 3U},
        {"0x 1", UMC_SYNTAX_ERROR, 0U}, {"0b/* comment */1", UMC_SYNTAX_ERROR, 0U},
        {"0x80000000", UMC_LITERAL_RANGE, 0U}, {"0xffffffff", UMC_LITERAL_RANGE, 0U},
        {"-0x80000000", UMC_LITERAL_RANGE, 1U},
        {"0b10000000000000000000000000000000", UMC_LITERAL_RANGE, 0U},
        {"0x00000000000080000000", UMC_LITERAL_RANGE, 0U}
    };
    const char prefix[] = "int main(void){return ";
    for (size_t i = 0U; i < sizeof cases / sizeof cases[0]; ++i) {
        char source[160];
        int length = snprintf(source, sizeof source, "%s%s;}", prefix, cases[i].text);
        CHECK(length > 0 && (size_t)length < sizeof source);
        UmcInstruction program[8]; UmcDiagnostic diagnostic; size_t count = 99U;
        CHECK(umc_compile(source, (size_t)length, program, 8U, &count, &diagnostic) == cases[i].status);
        CHECK(count == 0U);
        CHECK(diagnostic.source_offset == sizeof prefix - 1U + cases[i].offset);
        CHECK(diagnostic.line == 1U && diagnostic.column == diagnostic.source_offset + 1U);
    }
    /* Location mapping remains byte based and treats CRLF as one line break. */
    const char multiline[] = "int main(void){\r\n return 0b102;}";
    UmcInstruction program[8]; UmcDiagnostic diagnostic; size_t count = 99U;
    CHECK(umc_compile(multiline, sizeof multiline - 1U, program, 8U, &count, &diagnostic) == UMC_SYNTAX_ERROR);
    CHECK(count == 0U && diagnostic.line == 2U && diagnostic.column == 13U);
    return 0;
}

static int spans(void)
{
    /* This array deliberately has no NUL. Each proper prefix is incomplete;
     * the lexer may inspect only the borrowed length, even when more backing
     * storage exists. The complete span must compile without a C string. */
    /* The original initializer intentionally omitted its NUL, but GCC also
     * diagnoses that spelling under strict string initialization warnings.
     * Copy exactly the payload into the same bounded non-string array; this
     * keeps the memory contract without suppressing a compiler diagnostic. */
#if 0
    const char raw[sizeof("int main(void){return 0x2a+0b10;}") - 1U] =
        "int main(void){return 0x2a+0b10;}";
#endif
    const char fixture[] = "int main(void){return 0x2a+0b10;}";
    char raw[sizeof fixture - 1U];
    memcpy(raw, fixture, sizeof raw);
    UmcInstruction program[8]; UmcDiagnostic diagnostic; UmcResult result;
    size_t count = 99U;
    for (size_t length = 0U; length < sizeof raw; ++length) {
        CHECK(umc_compile(raw, length, program, 8U, &count, &diagnostic) != UMC_OK);
        CHECK(count == 0U && diagnostic.source_offset <= length);
    }
    CHECK(umc_compile(raw, sizeof raw, program, 8U, &count, &diagnostic) == UMC_OK);
    CHECK(umc_execute(program, count, count, &result, &diagnostic) == UMC_OK && result.value == 44);
    CHECK(umc_compile(raw, sizeof raw, program, 1U, &count, &diagnostic) == UMC_PROGRAM_TOO_LARGE);
    CHECK(count == 0U);
    /* Leading zeroes are bounded by the source limit, not a host conversion
     * buffer. Exactly-limit input is legal; a larger declared span is refused
     * before reading beyond this allocation. */
    char limit[UMC_SOURCE_LIMIT];
    const char prefix[] = "int main(void){return 0x";
    memcpy(limit, prefix, sizeof prefix - 1U);
    memset(limit + sizeof prefix - 1U, '0', sizeof limit - (sizeof prefix - 1U));
    memcpy(limit + sizeof limit - 3U, "1;}", 3U);
    CHECK(umc_compile(limit, sizeof limit, program, 8U, &count, &diagnostic) == UMC_OK);
    CHECK(count == 2U && program[0].operand == 1);
    CHECK(umc_compile(limit, sizeof limit + 1U, program, 8U, &count, &diagnostic) == UMC_SOURCE_TOO_LARGE);
    CHECK(count == 0U);
    char zero_byte[] = "int main(void){return 0x1Z;}";
    /* Preserve the mistaken fixture mutation for review: byte 24 is the
     * only digit, so removing it tests an empty prefix instead of an embedded
     * NUL after a digit. Byte 25 is the following Z in the explicit source. */
#if 0
    zero_byte[24] = '\0';
#endif
    zero_byte[25] = '\0';
    CHECK(umc_compile(zero_byte, sizeof zero_byte - 1U, program, 8U, &count, &diagnostic) == UMC_SYNTAX_ERROR);
    /* The former assertion belongs to the incorrectly located NUL above. */
#if 0
    CHECK(count == 0U && diagnostic.source_offset == 24U);
#endif
    CHECK(count == 0U && diagnostic.source_offset == 25U);
    return 0;
}

static int wire(void)
{
    /* Three spellings produce the same value at the same byte offset. This
     * literal fixture is independent of the encoder and includes format 1,
     * little-endian operand bytes, and both source offsets (22 and 15). */
    /* Literal text starts at byte 22. Retain the prior off-by-one golden
     * vector for review; this correction changes fixture evidence only. */
#if 0
    static const uint8_t expected[] = {
        'U','M','C','B',1,0,2,0,
        1,42,0,0,0,21,0,0,0,
        7,0,0,0,0,15,0,0,0
    };
#endif
    static const uint8_t expected[] = {
        'U','M','C','B',1,0,2,0,
        1,42,0,0,0,22,0,0,0,
        7,0,0,0,0,15,0,0,0
    };
    static const char *const source[] = {
        "int main(void){return 42;}", "int main(void){return 0x2a;}",
        "int main(void){return 0B101010;}"
    };
    for (size_t i = 0U; i < sizeof source / sizeof source[0]; ++i) {
        UmcInstruction program[8]; uint8_t bytes[128]; size_t count = 0U, size = 0U;
        CHECK(umc_compile(source[i], strlen(source[i]), program, 8U, &count, NULL) == UMC_OK);
        CHECK(umc_encode(program, count, bytes, sizeof bytes, &size, NULL) == UMC_OK);
        CHECK(size == sizeof expected && memcmp(bytes, expected, size) == 0);
    }
    return 0;
}

static int decimal(void)
{
    CHECK(evaluate("010", 10) == 0);
    CHECK(evaluate("0009", 9) == 0);
    CHECK(evaluate("00000000002147483647", INT32_MAX) == 0);
    CHECK(evaluate("-2147483647-1", INT32_MIN) == 0);
    const char source[] = "int main(void){return 2147483648;}";
    UmcInstruction program[8]; UmcDiagnostic diagnostic; size_t count = 99U;
    CHECK(umc_compile(source, sizeof source - 1U, program, 8U, &count, &diagnostic) == UMC_LITERAL_RANGE);
    /* Preserve the original off-by-one expectation; return plus its space
     * occupies seven bytes beginning at byte 15, so the token begins at 22. */
#if 0
    CHECK(count == 0U && diagnostic.source_offset == 21U);
#endif
    CHECK(count == 0U && diagnostic.source_offset == 22U);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (strcmp(argv[1], "values") == 0) return values();
    if (strcmp(argv[1], "refusals") == 0) return refusals();
    if (strcmp(argv[1], "spans") == 0) return spans();
    if (strcmp(argv[1], "wire") == 0) return wire();
    if (strcmp(argv[1], "decimal") == 0) return decimal();
    return 2;
}
