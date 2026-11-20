/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_counted_for.c
 *
 * PURPOSE:
 *   Validate bounded loop lowering, lexical scope and the existing forward runtime.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "test.h"
#include "umicom_c/int_input.h"
#include "umicom_c/source_check.h"

static UmcStatus Compile(const char *body, UmcInstruction *code, size_t capacity,
    size_t *count, UmcDiagnostic *diagnostic)
{
    /* Tests vary statements without replacing the ordinary main entry grammar. */
    char source[2048];
    int length = snprintf(source, sizeof source, "int main(void){%s}", body);
    if (length < 0 || (size_t)length >= sizeof source) return UMC_INVALID_ARGUMENT;
    return umc_compile(source, (size_t)length, code, capacity, count, diagnostic);
}

static int Values(void)
{
    /* Exercise separate local scopes, nested lowering, literal limits and
     * compatibility with the former ordinary spelling of 'for'. */
    const struct { const char *body; int32_t value; } cases[] = {
        {"int s=0;for(int i=0;i<4;++i){s+=i;}return s;", 6},
        {"int s=0;for(int i=0;i<2;++i){for(int j=0;j<3;++j){s+=i*10+j;}}return s;", 36},
        {"int s=0;if(1){for(int i=0x2;i<0b101;++i){int n=i;s+=n;}}return s;", 9},
        {"int s=0;for(int i=1;i<17;++i){++s;}return s;", 16},
        {"for(int i=2147483646;i<2147483647;++i){}return 42;", 42},
        {"for(int i=0;i<1;++i){int x=i;}int i=42;return i;", 42},
        {"int for=6;for+=36;return for;", 42},
        {"int r=0;for /* comment */ (int k=0;k<3;++k){r+=14;}return r;", 42}
    };
    for (size_t index = 0U; index < sizeof cases / sizeof cases[0]; ++index) {
        UmcInstruction code[UMC_PROGRAM_LIMIT];
        size_t count = 0U;
        UmcResult result;
        UmcDiagnostic diagnostic;
        CHECK(Compile(cases[index].body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
        CHECK(umc_execute(code, count, count, &result, &diagnostic) == UMC_OK);
        CHECK(result.value == cases[index].value);
    }
    return 0;
}

static int Refusals(void)
{
    /* Even an unselected runtime branch must obey the bounded source profile. */
    const char *cases[] = {
        "for(int i=0;i<0;++i){}return 0;",
        "for(int i=2;i<1;++i){}return 0;",
        "for(int i=0;i<17;++i){}return 0;",
        "for(int i=0;j<3;++i){}return 0;",
        "for(int i=0;i<3;++j){}return 0;",
        "for(int i=0;i<3;--i){}return 0;",
        "int n=3;for(int i=0;i<n;++i){}return 0;",
        "for(int i='a';i<3;++i){}return 0;",
        "for(int i=0;i<true;++i){}return 0;",
        "for(int i=0;i<3;++i)i++;return 0;",
        "if(0){for(int i=0;i<3;++i){i+=1;}}return 0;",
        "for(int i=0;i<3;++i){}return i;",
        "int i=1;for(int i=0;i<3;++i){}return i;",
        "for(int i=0;i<3;++i){int i=4;}return 0;",
        "for(int i=0;i<3;++i){return 0;}return 0;"
    };
    for (size_t index = 0U; index < sizeof cases / sizeof cases[0]; ++index) {
        UmcInstruction code[UMC_PROGRAM_LIMIT];
        UmcDiagnostic diagnostic;
        size_t count = 77U;
        CHECK(Compile(cases[index], code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        CHECK(count == 0U && diagnostic.source_offset < strlen(cases[index]) + 17U);
    }
    return 0;
}

static int Wire(void)
{
    const char *body =
        "int s=0;for(int i=0;i<3;++i){if(i==1){s+=41;}else{s+=(0&&(1/0));}}return s+1;";
    UmcInstruction code[UMC_PROGRAM_LIMIT], decoded[UMC_PROGRAM_LIMIT];
    size_t count = 0U, decoded_count = 0U, written = 0U;
    UmcDiagnostic diagnostic;
    uint8_t bytes[UMC_BYTECODE_LIMIT];
    CHECK(Compile(body, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    size_t branches = 0U;
    /* Existing bytecode consumers accept every cloned branch in its new
     * forward coordinates, including a jump to the next iteration. */
    for (size_t index = 0U; index < count; ++index) {
        if (code[index].opcode == UMC_JUMP || code[index].opcode == UMC_JUMP_IF_ZERO) {
            CHECK(code[index].operand >= 0);
            CHECK((size_t)code[index].operand > index && (size_t)code[index].operand < count);
            ++branches;
        }
    }
    CHECK(branches >= 9U);
    CHECK(umc_encode(code, count, bytes, sizeof bytes, &written, &diagnostic) == UMC_OK);
    CHECK(bytes[4] == 4U);
    CHECK(umc_decode(bytes, written, decoded, UMC_PROGRAM_LIMIT, &decoded_count, &diagnostic) == UMC_OK);
    CHECK(decoded_count == count);
    UmcResult result;
    CHECK(umc_execute(decoded, decoded_count, decoded_count, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == 42 && result.instructions_executed < count);
    const size_t fuel = result.instructions_executed;
    result.value = 7;
    CHECK(umc_execute(decoded, decoded_count, fuel - 1U, &result, &diagnostic) == UMC_FUEL_EXHAUSTED);
    CHECK(result.value == 7);
    return 0;
}

static int Bounds(void)
{
    /* Source that expands past either capacity must leave no runnable count. */
    UmcInstruction code[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    size_t count = 99U;
    CHECK(Compile("int s=0;for(int i=0;i<4;++i){s+=i;}return s;", code, 8U, &count, &diagnostic) ==
        UMC_PROGRAM_TOO_LARGE);
    CHECK(count == 0U);
    CHECK(Compile("int s=0;for(int i=0;i<16;++i){s+=i+i+i+i+i+i+i+i+i+i;}return s;",
        code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_PROGRAM_TOO_LARGE);
    CHECK(count == 0U);
    /* A length-delimited source can end in the header with no trailing NUL. */
    static const char incomplete[] = "int main(void){for(int i=";
    char raw[sizeof incomplete - 1U];
    memcpy(raw, incomplete, sizeof raw);
    CHECK(umc_compile(raw, sizeof raw, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
    CHECK(count == 0U && diagnostic.source_offset == sizeof raw);
    return 0;
}

static int Warnings(void)
{
    /* One lexical declaration is one finding, even when lowered five times. */
    const char *source = "int main(void){for(int i=0;i<5;++i){int unused=i;}return 42;}";
    UmcSourceReport report;
    UmcDiagnostic diagnostic;
    CHECK(umc_check_source(source, strlen(source), &report, &diagnostic) == UMC_OK);
    CHECK(report.warning_count == 1U);
    CHECK(report.warnings[0].kind == UMC_WARNING_LOCAL_NEVER_READ);
    CHECK(report.warnings[0].source_offset == (size_t)(strstr(source, "unused") - source));
    CHECK(report.warnings[0].source_length == sizeof "unused" - 1U);
    return 0;
}

static int HostInput(void)
{
    /* The optional web/SDK entry uses the same parser. An input-dependent
     * body reuses the original parameter across independent bindings. */
    const char *source = "int umicom_run(int step){int r=0;for(int i=0;i<3;++i){r+=step+i;}return r;}";
    uint8_t bytes[UMC_BYTECODE_LIMIT], original[UMC_BYTECODE_LIMIT];
    size_t length = 0U;
    UmcDiagnostic diagnostic;
    CHECK(umc_compile_int_input(source, strlen(source), bytes, sizeof bytes, &length, &diagnostic) == UMC_OK);
    CHECK(bytes[5] == UMC_INT_INPUT_BYTECODE_FLAG);
    memcpy(original, bytes, length);
    const int32_t input[] = {13, -2}, expected[] = {42, -3};
    for (size_t index = 0U; index < sizeof input / sizeof input[0]; ++index) {
        UmcInstruction code[UMC_PROGRAM_LIMIT];
        size_t count = 0U;
        UmcResult result;
        CHECK(umc_bind_int_input_bytecode(bytes, length, input[index], code, UMC_PROGRAM_LIMIT, &count,
            &diagnostic) == UMC_OK);
        CHECK(umc_execute(code, count, count, &result, &diagnostic) == UMC_OK && result.value == expected[index]);
        CHECK(memcmp(bytes, original, length) == 0);
    }
    return 0;
}

int main(int argc, char **argv)
{
    /* Each CTest registration selects one independent observable contract. */
    CHECK(argc == 2);
    if (strcmp(argv[1], "values") == 0) return Values();
    if (strcmp(argv[1], "refusals") == 0) return Refusals();
    if (strcmp(argv[1], "wire") == 0) return Wire();
    if (strcmp(argv[1], "bounds") == 0) return Bounds();
    if (strcmp(argv[1], "warnings") == 0) return Warnings();
    if (strcmp(argv[1], "host-input") == 0) return HostInput();
    return 2;
}
