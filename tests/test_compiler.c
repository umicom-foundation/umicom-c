/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_compiler.c
 *
 * PURPOSE:
 *   Exercise precedence, source boundaries, diagnostics and rejected language constructs.
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

int main(void)
{
    static const struct {
        const char *source;
        int32_t expected;
    } cases[] = {
        {"int main(void) { return 2 + 3 * 4; }", 14},
        {"int main(void) { return (2 + 3) * 4; }", 20},
        {"int main(void) { return 20 / 5 / 2; }", 2},
        {"int main(void) { return 10 - 3 - 2; }", 5},
        {"int main(void) { return -7 / 2; }", -3},
        {"int main(void) { return +--7; }", 7},
        {"int/**/main(void) { // line\n return 40 /* block */ + 2; }", 42},
        {"int main(void) { return (-2147483647 - 1); }", INT32_MIN}
    };
    UmcInstruction program[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        size_t count = 99U;
        CHECK(umc_compile(cases[i].source, strlen(cases[i].source),
            program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
        UmcResult result;
        CHECK(umc_execute(program, count, count, &result, &diagnostic) == UMC_OK);
        CHECK(result.value == cases[i].expected);
        CHECK(result.instructions_executed == count);
    }
    static const char *const invalid[] = {
        "", "int main() { return 1; }", "int main(void) { return 1 }",
        "int main(void) { return 1; } garbage", "int main(void) { return 1 2; }",
        "int main(void) { return 1/**/2; }", "int main(void) { return (1 +); }",
        "int main(void) { return foo; }", "int main(void) { return 0x10; }",
        "int main(void) { return 1; } /* missing", "#define X 1\nint main(void){return X;}"
    };
    for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        size_t count = 99U;
        CHECK(umc_compile(invalid[i], strlen(invalid[i]), program,
            UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        CHECK(count == 0U);
        CHECK(diagnostic.line >= 1U && diagnostic.column >= 1U);
    }
    const char exact[] = {'i','n','t',' ','m','a','i','n','(','v','o','i','d',')','{',
                          'r','e','t','u','r','n',' ','4','2',';','}'};
    size_t count = 0U;
    CHECK(umc_compile(exact, sizeof(exact), program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    const char hidden[] = "int main(void){return 1;}\0int main(void){return 2;}";
    CHECK(umc_compile(hidden, sizeof(hidden) - 1U, program, UMC_PROGRAM_LIMIT,
        &count, &diagnostic) == UMC_SYNTAX_ERROR);
    const char overflow[] = "int main(void){return 2147483648;}";
    CHECK(umc_compile(overflow, sizeof(overflow) - 1U, program, UMC_PROGRAM_LIMIT,
        &count, &diagnostic) == UMC_LITERAL_RANGE);
    const char location[] = "int main(void) {\r\nreturn @;\r\n}";
    CHECK(umc_compile(location, sizeof(location) - 1U, program, UMC_PROGRAM_LIMIT,
        &count, &diagnostic) == UMC_SYNTAX_ERROR);
    CHECK(diagnostic.line == 2U && diagnostic.column == 8U);
    const char simple[] = "int main(void){return 42;}";
    CHECK(umc_compile(simple, sizeof(simple) - 1U, program, 1U, &count, &diagnostic) == UMC_PROGRAM_TOO_LARGE);
    CHECK(count == 0U);
    CHECK(umc_compile(simple, UMC_SOURCE_LIMIT + 1U, program, UMC_PROGRAM_LIMIT,
        &count, &diagnostic) == UMC_SOURCE_TOO_LARGE);
    CHECK(umc_compile(NULL, 0U, program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_INVALID_ARGUMENT);
    char nested[256];
    const char prefix[] = "int main(void){return ";
    size_t length = sizeof(prefix) - 1U;
    memcpy(nested, prefix, length);
    for (size_t i = 0U; i < UMC_PARSE_DEPTH_LIMIT + 1U; ++i) nested[length++] = '-';
    nested[length++] = '1'; nested[length++] = ';'; nested[length++] = '}';
    CHECK(umc_compile(nested, length, program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_DEPTH_LIMIT);
    CHECK(count == 0U);
    return 0;
}
