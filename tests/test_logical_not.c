/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_logical_not.c
 *
 * PURPOSE:
 *   Verify negation values, wire compatibility, trace identity and refusals.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "test.h"
#include "umicom_c/trace.h"

static int values(void)
{
    static const struct { const char *text; int32_t expected; } cases[] = {
        {"!0", 1}, {"!1", 0}, {"!-7", 0}, {"!2147483647", 0},
        {"!(-2147483647 - 1)", 0}, {"!!0", 0}, {"!!-9", 1},
        {"!!!0", 1}, {"!0 + 4", 5}, {"!(0 + 4)", 0},
        {"!0 * 4", 4}, {"-!0", -1}, {"+!0", 1},
        {"!1 < 1", 1}, {"!(1 < 1)", 1}, {"!0 == 1", 1},
        {"1 != !0", 0}, {"!/* comment */0", 1}, {"! // line\n0", 1}
    };
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        char source[160];
        int n = snprintf(source, sizeof(source), "int main(void){return %s;}", cases[i].text);
        CHECK(n > 0 && (size_t)n < sizeof(source));
        UmcInstruction program[UMC_PROGRAM_LIMIT], decoded[UMC_PROGRAM_LIMIT];
        uint8_t wire[UMC_BYTECODE_LIMIT];
        size_t count = 0U, bytes = 0U, restored = 0U;
        UmcDiagnostic diagnostic;
        UmcResult result;
        CHECK(umc_compile(source, (size_t)n, program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
        CHECK(umc_execute(program, count, count, &result, &diagnostic) == UMC_OK);
        CHECK(result.value == cases[i].expected);
        CHECK(umc_encode(program, count, wire, sizeof(wire), &bytes, &diagnostic) == UMC_OK);
        CHECK(wire[4] == 2U);
        CHECK(umc_decode(wire, bytes, decoded, UMC_PROGRAM_LIMIT, &restored, &diagnostic) == UMC_OK);
        CHECK(restored == count);
        CHECK(umc_execute(decoded, restored, restored, &result, &diagnostic) == UMC_OK);
        CHECK(result.value == cases[i].expected);
    }
    return 0;
}
static int lowered_trace(void)
{
    const char source[] = "int main(void){return !0;}";
    const size_t not_offset = (size_t)(strchr(source, '!') - source);
    UmcInstruction program[8];
    UmcDiagnostic diagnostic;
    size_t count = 0U;
    CHECK(umc_compile(source, sizeof(source) - 1U, program, 8U, &count, &diagnostic) == UMC_OK);
    CHECK(count == 4U && program[0].opcode == UMC_PUSH && program[0].operand == 0);
    CHECK(program[1].opcode == UMC_PUSH && program[1].operand == 0);
    CHECK(program[2].opcode == UMC_EQUAL && program[3].opcode == UMC_RETURN);
    CHECK(program[1].source_offset == not_offset && program[2].source_offset == not_offset);
    UmcProgramInfo info;
    CHECK(umc_inspect(program, count, &info, &diagnostic) == UMC_OK);
    CHECK(info.peak_stack == 2U && info.constant_count == 2U && info.instruction_count == 4U);
    UmcTraceStep steps[4]; size_t used = 0U;
    UmcResult result = {93, 71U};
    CHECK(umc_execute_trace(program, count, 4U, &result, steps, 4U, &used, &diagnostic) == UMC_OK);
    CHECK(used == 4U && result.value == 1 && result.instructions_executed == 4U);
    CHECK(steps[1].stack_depth == 2U && steps[2].stack_depth == 1U && steps[2].top == 1);
    result.value = 93; result.instructions_executed = 71U;
    CHECK(umc_execute_trace(program, count, 2U, &result, steps, 4U, &used, &diagnostic) == UMC_FUEL_EXHAUSTED);
    CHECK(result.value == 93 && result.instructions_executed == 71U);
    CHECK(diagnostic.source_offset == not_offset);
    CHECK(umc_execute_trace(program, count, 4U, &result, steps, 3U, &used, &diagnostic) == UMC_BUFFER_TOO_SMALL);
    CHECK(used == 0U && result.value == 93 && result.instructions_executed == 71U);
    return 0;
}
static int operand_failure(void)
{
    const char source[] = "int main(void){return !(1/0);}";
    UmcInstruction program[8]; UmcDiagnostic diagnostic;
    size_t count = 0U, used = 0U;
    CHECK(umc_compile(source, sizeof(source) - 1U, program, 8U, &count, &diagnostic) == UMC_OK);
    UmcTraceStep steps[8]; UmcResult result = {17, 23U};
    CHECK(umc_execute_trace(program, count, count, &result, steps, 8U, &used, &diagnostic) == UMC_ARITHMETIC_ERROR);
    CHECK(used == 3U && steps[2].opcode == UMC_DIVIDE);
    CHECK(diagnostic.source_offset == (size_t)(strchr(source, '/') - source));
    CHECK(result.value == 17 && result.instructions_executed == 23U);
    return 0;
}
static int refusals(void)
{
    static const char *const invalid[] = {
        "int main(void){return !;}", "int main(void){return 1 ! 0;}",
        "int main(void){return ! = 0;}", "int main(void){return !=0;}",
        /* Logical pairs are supported; retain old refusals and exercise
         * the still-unsupported single-character bitwise operators. */
#if 0
        "int main(void){return 1 && !0;}", "int main(void){return 1 || !0;}"
#endif
        "int main(void){return 1 & !0;}", "int main(void){return 1 | !0;}"
    };
    UmcInstruction program[UMC_PROGRAM_LIMIT]; UmcDiagnostic diagnostic;
    size_t count = 99U;
    for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        CHECK(umc_compile(invalid[i], strlen(invalid[i]), program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        CHECK(count == 0U);
    }
    /* The borrowed span ends at !; storage beyond it is not source input. */
    const char span[] = "int main(void){return !0;}";
    size_t length = (size_t)(strchr(span, '!') - span) + 1U;
    CHECK(umc_compile(span, length, program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
    CHECK(count == 0U && diagnostic.source_offset == length);
    CHECK(umc_compile(span, sizeof(span) - 1U, program, 3U, &count, &diagnostic) == UMC_PROGRAM_TOO_LARGE);
    CHECK(count == 0U);
    char nested[160] = "int main(void){return ";
    const size_t prefix = strlen(nested);
    memset(nested + prefix, '!', UMC_PARSE_DEPTH_LIMIT - 1U);
    memcpy(nested + prefix + UMC_PARSE_DEPTH_LIMIT - 1U, "0;}", 3U);
    CHECK(umc_compile(nested, prefix + UMC_PARSE_DEPTH_LIMIT + 2U, program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    CHECK(count == 2U * UMC_PARSE_DEPTH_LIMIT);
    memset(nested + prefix, '!', UMC_PARSE_DEPTH_LIMIT);
    memcpy(nested + prefix + UMC_PARSE_DEPTH_LIMIT, "0;}", 3U);
    CHECK(umc_compile(nested, prefix + UMC_PARSE_DEPTH_LIMIT + 3U, program, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_DEPTH_LIMIT);
    CHECK(count == 0U);
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (strcmp(argv[1], "values") == 0) return values();
    if (strcmp(argv[1], "trace") == 0) return lowered_trace();
    if (strcmp(argv[1], "operand-failure") == 0) return operand_failure();
    if (strcmp(argv[1], "refusals") == 0) return refusals();
    return 2;
}
