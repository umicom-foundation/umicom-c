/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_compound_statements.c
 *
 * PURPOSE:
 *   Verify standalone lexical scopes, bounded nesting, bytecode compatibility and source error boundaries.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


#include "test.h"
#include "umicom_c/profile.h"
#include <stdarg.h>

static UmcStatus compile_body(const char *body, UmcInstruction *code,
    size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    char source[8192];
    int n = snprintf(source, sizeof source, "int main(void){%s}", body);
    if (n < 0 || (size_t)n >= sizeof source) return UMC_INVALID_ARGUMENT;
    return umc_compile(source, (size_t)n, code, capacity, count, diagnostic);
}
static int append(char *text, size_t capacity, size_t *used, const char *format, ...)
{
    va_list args; va_start(args, format);
    int n = vsnprintf(text + *used, capacity - *used, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= capacity - *used) return 1;
    *used += (size_t)n;
    return 0;
}
static int values(void)
{
    const struct { const char *body; int32_t value; } cases[] = {
        {"{}return 42;", 42},
        {"{int x=1;}return 0;", 0},
        {"{int x=1;} return 0;", 0},
        {"int x=1;if(1){{x=2;}}return x;", 2},
        {"int x=1;if(0){}else if(1){{x=2;}}return x;", 2},
        {"int r=0;{const int answer=42;r=answer;}return r;", 42},
        {"int r=0;{{{int x=40;r=x;}}r+=2;}return r;", 42},
        {"int r=0;{int x=6;{int y=7;r=x*y;}x++;}return r;", 42},
        {"int r=0;{int x=7;r=x;}{int x=6;r*=x;}return r;", 42},
        {"int r=0;{int x=2;if(x){const int y=40;r=x+y;}}return r;", 42},
        {"int r=0;if(1){{int x=42;r=x;}}return r;", 42},
        {"int r=0;if(0){{int x=1/0;r=x;}}else{{int x=42;r=x;}}return r;", 42},
        {"int r=0;{int x=7;r=x;}int x=6;r*=x;return r;", 42},
        {"int r=0;{int const x=42;r=x;}return r;", 42},
        {"int r=41;{{++r;}r++;}r--;return r;", 42}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 0;
        UmcDiagnostic diagnostic; UmcResult result;
        CHECK(compile_body(cases[i].body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
        CHECK(umc_execute(code, count, count, &result, &diagnostic) == UMC_OK);
        CHECK(result.value == cases[i].value);
    }
    return 0;
}
static int visibility(void)
{
    const char *bad[] = {
        "{int x=1;}return x;", "int r=0;{int x=1;}{r=x;}return r;",
        "int x=1;{int x=2;}return x;", "{int x=1;{int x=2;}}return 0;",
        "{int x=1;int x=2;}return 0;", "{int x=x;}return 0;",
        "{int x;}return 0;", "int r=0;{{int x=1;}r=x;}return r;",
        "{const int x=1;x=2;}return 0;", "if(0){{const int x=1;++x;}}return 0;"
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 91; UmcDiagnostic diagnostic;
        CHECK(compile_body(bad[i], code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        CHECK(count == 0);
    }
    return 0;
}
static int refusals(void)
{
/* Empty statements are now admitted by the compiler and checked in the dedicated empty-statement suite. Preserve the former refusal inventory for grammar-history review. The former implementation is retained for owner review. */
#if 0
    const char *bad[] = {
        "{return 42;}return 0;", "{int x=1;", "}return 0;", "{;}return 0;",
        "{int x=1;}else{}return 0;", "{if(1)int x=2;}return 0;",
        "{while(0){}}return 0;", "{int x=1;break;}return 0;"
    };

#endif
    const char *bad[] = {
        "{return 42;}return 0;", "{int x=1;", "}return 0;",
        "{int x=1;}else{}return 0;", "{if(1)int x=2;}return 0;",
        "{while(0){}}return 0;", "{int x=1;break;}return 0;"
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 91; UmcDiagnostic diagnostic;
        CHECK(compile_body(bad[i], code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        CHECK(count == 0);
    }
    return 0;
}
static int limits(void)
{
    for (size_t locals = UMC_LOCAL_LIMIT; locals <= UMC_LOCAL_LIMIT + 1U; ++locals) {
        char body[8192] = ""; size_t used = 0;
        CHECK(!append(body, sizeof body, &used, "{"));
        for (size_t i = 0; i < locals; ++i)
            CHECK(!append(body, sizeof body, &used, "int x%zu=%zu;", i, i));
        CHECK(!append(body, sizeof body, &used, "}return 42;"));
        UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 91; UmcDiagnostic diagnostic;
        UmcStatus status = compile_body(body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic);
        if (locals == UMC_LOCAL_LIMIT) CHECK(status == UMC_OK);
        else CHECK(status == UMC_LOCAL_CAPACITY && count == 0);
    }
    UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 0; UmcDiagnostic diagnostic;
    const char *body = "int r=0;{const int x=42;r=x;}return r;";
    CHECK(compile_body(body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK && count == 8U);
    CHECK(compile_body(body, code, 7U, &count, &diagnostic) == UMC_PROGRAM_TOO_LARGE && count == 0U);
    return 0;
}
static int reuse(void)
{
    char body[8192] = ""; size_t used = 0;
    CHECK(!append(body, sizeof body, &used, "int r=0;"));
    for (size_t scope = 0; scope < 3U; ++scope) {
        CHECK(!append(body, sizeof body, &used, "{"));
        for (size_t i = 0; i < 30U; ++i)
            CHECK(!append(body, sizeof body, &used, "int x%zu=%zu;", i, i));
        CHECK(!append(body, sizeof body, &used, "r+=x29;}"));
    }
    CHECK(!append(body, sizeof body, &used, "return r;"));
    UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 0; UmcDiagnostic diagnostic; UmcResult result;
    CHECK(compile_body(body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    CHECK(umc_execute(code, count, count, &result, &diagnostic) == UMC_OK && result.value == 87);
    for (size_t i = 0; i < count; ++i)
        if (code[i].opcode == UMC_STORE_LOCAL || code[i].opcode == UMC_LOAD_LOCAL)
            CHECK(code[i].operand >= 0 && code[i].operand < 31);
    return 0;
}
static int depth(void)
{
    /* Empty bodies isolate structural depth from expression-recursion cost. */
    for (size_t nesting = UMC_PARSE_DEPTH_LIMIT; nesting <= UMC_PARSE_DEPTH_LIMIT + 1U; ++nesting) {
        char body[8192] = ""; size_t used = 0;
        for (size_t i = 0; i < nesting; ++i) CHECK(!append(body, sizeof body, &used, "{"));
        for (size_t i = 0; i < nesting; ++i) CHECK(!append(body, sizeof body, &used, "}"));
        CHECK(!append(body, sizeof body, &used, "return 42;"));
        UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 91; UmcDiagnostic diagnostic;
        UmcStatus status = compile_body(body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic);
        if (nesting == UMC_PARSE_DEPTH_LIMIT) CHECK(status == UMC_OK && count == 2U);
        else CHECK(status == UMC_DEPTH_LIMIT && count == 0U);
    }
    char body[8192] = ""; size_t used = 0;
    for (size_t i = 0; i < UMC_PARSE_DEPTH_LIMIT + 1U; ++i)
        CHECK(!append(body, sizeof body, &used, "%s", i % 2U ? "if(1){" : "{"));
    for (size_t i = 0; i < UMC_PARSE_DEPTH_LIMIT + 1U; ++i)
        CHECK(!append(body, sizeof body, &used, "}"));
    CHECK(!append(body, sizeof body, &used, "return 0;"));
    UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 91; UmcDiagnostic diagnostic;
    CHECK(compile_body(body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_DEPTH_LIMIT && count == 0);
    return 0;
}
static int wire(void)
{
    const char *body = "int r=0;{const int x=42;r=x;}return r;";
    const UmcOpcode opcodes[] = {UMC_PUSH, UMC_STORE_LOCAL, UMC_PUSH, UMC_STORE_LOCAL,
        UMC_LOAD_LOCAL, UMC_STORE_LOCAL, UMC_LOAD_LOCAL, UMC_RETURN};
    const int32_t operands[] = {0, 0, 42, 1, 1, 0, 0, 0};
    UmcInstruction code[UMC_PROGRAM_LIMIT], decoded[UMC_PROGRAM_LIMIT];
    size_t count = 0, written = 0, decoded_count = 0; uint8_t bytes[UMC_BYTECODE_LIMIT];
    UmcDiagnostic diagnostic; UmcResult result;
    CHECK(compile_body(body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK && count == 8U);
    for (size_t i = 0; i < count; ++i)
        CHECK(code[i].opcode == opcodes[i] && code[i].operand == operands[i]);
    CHECK(umc_encode(code, count, bytes, sizeof bytes, &written, &diagnostic) == UMC_OK);
    CHECK(bytes[4] == 3U);
    CHECK(umc_decode(bytes, written, decoded, UMC_PROGRAM_LIMIT, &decoded_count, &diagnostic) == UMC_OK);
    CHECK(decoded_count == count);
    for (size_t fuel = 0; fuel <= count; ++fuel) {
        result.value = 73; result.instructions_executed = 91;
        UmcStatus status = umc_execute(decoded, decoded_count, fuel, &result, &diagnostic);
        if (fuel < count) CHECK(status == UMC_FUEL_EXHAUSTED && result.value == 73 && result.instructions_executed == 91U);
        else CHECK(status == UMC_OK && result.value == 42 && result.instructions_executed == 8U);
    }
    return 0;
}
static int faults(void)
{
    const char *bad[] = {"int r=0;{int x=1/0;r=x;}return r;",
        "int r=0;{{const int x=2147483647;x++;}}return r;"};
    UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 0; UmcDiagnostic diagnostic;
    UmcResult result = {73, 91};
    CHECK(compile_body(bad[0], code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    CHECK(umc_execute(code, count, count, &result, &diagnostic) == UMC_ARITHMETIC_ERROR);
    CHECK(result.value == 73 && result.instructions_executed == 91U);
    /* The second input violates const before any runtime overflow can execute. */
    CHECK(compile_body(bad[1], code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR && count == 0U);
    CHECK(compile_body("int r=0;{{int x=2147483647;x++;}}return r;", code, UMC_PROGRAM_LIMIT,
        &count, &diagnostic) == UMC_OK);
    CHECK(umc_execute(code, count, count, &result, &diagnostic) == UMC_ARITHMETIC_ERROR);
    CHECK(result.value == 73 && result.instructions_executed == 91U);
    return 0;
}
static int spans(void)
{
    const char source[] = "int main(void){\r\n{int hidden=1;}\r\nreturn hidden;}";
    UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 0; UmcDiagnostic diagnostic;
    CHECK(umc_compile(source, sizeof source - 1U, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
    CHECK(count == 0U && diagnostic.line == 3U && diagnostic.column == 8U);
    const char valid[] = "int main(void){int r=0;{{int x=42;r=x;}}return r;}";
    char exact[sizeof valid - 1U]; memcpy(exact, valid, sizeof exact);
    CHECK(umc_compile(exact, sizeof exact, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    for (size_t length = 0; length < sizeof exact; ++length)
        CHECK(umc_compile(exact, length, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) != UMC_OK && count == 0U);
    return 0;
}
static int native(void)
{
    for (int input = -20; input <= 20; ++input) {
        int expected = input;
        { int delta = input * 2; { const int scale = 3; expected += delta * scale; } }
        { const int delta = 2; expected -= delta; }
        char body[256];
        int n = snprintf(body, sizeof body,
            "int r=%d;{int delta=r*2;{const int scale=3;r+=delta*scale;}}{const int delta=2;r-=delta;}return r;", input);
        CHECK(n > 0 && (size_t)n < sizeof body);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count = 0; UmcDiagnostic diagnostic; UmcResult result;
        CHECK(compile_body(body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
        CHECK(umc_execute(code, count, count, &result, &diagnostic) == UMC_OK && result.value == expected);
    }
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    CHECK(UMC_COMPOUND_STATEMENT_PROFILE == 1U);
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES) & UMC_FEATURE_COMPOUND_STATEMENTS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES) & UMC_FEATURE_COMPOUND_STATEMENTS));
    if (!strcmp(argv[1], "values")) return values();
    if (!strcmp(argv[1], "visibility")) return visibility();
    if (!strcmp(argv[1], "refusals")) return refusals();
    if (!strcmp(argv[1], "limits")) return limits();
    if (!strcmp(argv[1], "reuse")) return reuse();
    if (!strcmp(argv[1], "depth")) return depth();
    if (!strcmp(argv[1], "wire")) return wire();
    if (!strcmp(argv[1], "faults")) return faults();
    if (!strcmp(argv[1], "spans")) return spans();
    if (!strcmp(argv[1], "native")) return native();
    return 2;
}
