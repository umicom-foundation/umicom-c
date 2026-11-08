/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_trace.c
 *
 * PURPOSE:
 *   Check complete, failed and refused execution traces against independent arithmetic expectations.
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
#include <limits.h>

static const UmcInstruction good[] = {
    {UMC_PUSH, 7, 10U}, {UMC_PUSH, 6, 12U},
    {UMC_MULTIPLY, 0, 11U}, {UMC_RETURN, 0, 3U}
};

static int success(void)
{
    UmcTraceStep steps[5];
    memset(steps, 0x5a, sizeof(steps));
    unsigned char unused[sizeof(steps[4])];
    memcpy(unused, &steps[4], sizeof(unused));
    UmcResult result = {19, 23U};
    UmcDiagnostic diagnostic;
    size_t used = 99U;
    CHECK(umc_execute_trace(good, 4U, 4U, &result, steps, 5U, &used, &diagnostic) == UMC_OK);
    CHECK(used == 4U && result.value == 42 && result.instructions_executed == 4U);
    CHECK(diagnostic.status == UMC_OK);
    const size_t depths[] = {1U, 2U, 1U, 1U};
    const int32_t tops[] = {7, 6, 42, 42};
    for (size_t i = 0U; i < used; ++i) {
        CHECK(steps[i].instruction_index == i && steps[i].source_offset == good[i].source_offset);
        CHECK(steps[i].opcode == good[i].opcode && steps[i].status == UMC_OK);
        CHECK(steps[i].stack_depth == depths[i] && steps[i].top == tops[i]);
    }
    CHECK(memcmp(unused, &steps[4], sizeof(unused)) == 0);
    return 0;
}

static int fuel_boundaries(void)
{
    const size_t depths[] = {0U, 1U, 2U, 1U};
    const int32_t tops[] = {0, 7, 6, 42};
    for (size_t fuel = 0U; fuel < 4U; ++fuel) {
        UmcTraceStep steps[4];
        UmcResult result = {19, 23U}, ordinary = result;
        UmcDiagnostic diagnostic, ordinary_diagnostic;
        size_t used = 99U;
        CHECK(umc_execute_trace(good, 4U, fuel, &result, steps, 4U, &used, &diagnostic) == UMC_FUEL_EXHAUSTED);
        CHECK(umc_execute(good, 4U, fuel, &ordinary, &ordinary_diagnostic) == UMC_FUEL_EXHAUSTED);
        CHECK(result.value == 19 && result.instructions_executed == 23U);
        CHECK(ordinary.value == result.value && ordinary.instructions_executed == result.instructions_executed);
        CHECK(used == fuel + 1U && steps[fuel].instruction_index == fuel);
        CHECK(steps[fuel].status == UMC_FUEL_EXHAUSTED && steps[fuel].stack_depth == depths[fuel]);
        CHECK(steps[fuel].top == tops[fuel] && diagnostic.source_offset == good[fuel].source_offset);
        CHECK(ordinary_diagnostic.source_offset == diagnostic.source_offset);
        for (size_t i = 0U; i < fuel; ++i) CHECK(steps[i].status == UMC_OK);
    }
    return 0;
}

static int arithmetic(const char *name)
{
    static const struct {
        const char *name; int32_t left; int32_t right; UmcOpcode opcode;
        UmcStatus expected; int32_t value;
    } cases[] = {
        {"add-overflow", INT32_MAX, 1, UMC_ADD, UMC_ARITHMETIC_ERROR, 0},
        {"subtract-overflow", INT32_MIN, 1, UMC_SUBTRACT, UMC_ARITHMETIC_ERROR, 0},
        {"multiply-overflow", INT32_MIN, INT32_MIN, UMC_MULTIPLY, UMC_ARITHMETIC_ERROR, 0},
        {"divide-zero", 7, 0, UMC_DIVIDE, UMC_ARITHMETIC_ERROR, 0},
        {"divide-overflow", INT32_MIN, -1, UMC_DIVIDE, UMC_ARITHMETIC_ERROR, 0},
        {"negative-division", -7, 2, UMC_DIVIDE, UMC_OK, -3},
        {"minimum-add", INT32_MIN, 0, UMC_ADD, UMC_OK, INT32_MIN},
        {"maximum-subtract", INT32_MAX, 0, UMC_SUBTRACT, UMC_OK, INT32_MAX}
    };
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (strcmp(name, cases[i].name) != 0) continue;
        UmcInstruction program[] = {{UMC_PUSH, cases[i].left, 2U},
            {UMC_PUSH, cases[i].right, 4U}, {cases[i].opcode, 0, 9U}, {UMC_RETURN, 0, 1U}};
        UmcTraceStep steps[4];
        size_t used = 0U;
        UmcResult result = {19, 23U}, ordinary = result;
        UmcDiagnostic diagnostic;
        CHECK(umc_execute_trace(program, 4U, 4U, &result, steps, 4U, &used, &diagnostic) == cases[i].expected);
        CHECK(umc_execute(program, 4U, 4U, &ordinary, NULL) == cases[i].expected);
        CHECK(ordinary.value == result.value && ordinary.instructions_executed == result.instructions_executed);
        if (cases[i].expected == UMC_OK) {
            CHECK(used == 4U && result.value == cases[i].value && result.instructions_executed == 4U);
            CHECK(steps[2].stack_depth == 1U && steps[2].top == cases[i].value);
        } else {
            CHECK(used == 3U && result.value == 19 && result.instructions_executed == 23U);
            CHECK(steps[2].status == UMC_ARITHMETIC_ERROR && steps[2].stack_depth == 2U);
            CHECK(steps[2].top == cases[i].right && diagnostic.source_offset == 9U);
        }
        return 0;
    }
    CHECK(0);
}

static int unary(void)
{
    for (size_t i = 0U; i < 2U; ++i) {
        UmcInstruction program[] = {{UMC_PUSH, i == 0U ? -42 : INT32_MIN, 1U},
            {UMC_NEGATE, 0, 2U}, {UMC_RETURN, 0, 0U}};
        UmcTraceStep steps[3];
        UmcResult result = {19, 23U};
        size_t used = 0U;
        const UmcStatus expected = i == 0U ? UMC_OK : UMC_ARITHMETIC_ERROR;
        CHECK(umc_execute_trace(program, 3U, 3U, &result, steps, 3U, &used, NULL) == expected);
        CHECK(steps[1].stack_depth == 1U && steps[1].status == expected);
        CHECK(steps[1].top == (i == 0U ? 42 : INT32_MIN));
        CHECK(used == (i == 0U ? 3U : 2U));
        CHECK(result.value == (i == 0U ? 42 : 19));
        CHECK(result.instructions_executed == (i == 0U ? 3U : 23U));
    }
    return 0;
}

static int refused(const char *name)
{
    UmcInstruction program[4];
    memcpy(program, good, sizeof(program));
    UmcTraceStep steps[4];
    memset(steps, 0x5a, sizeof(steps));
    unsigned char before[sizeof(steps)];
    memcpy(before, steps, sizeof(before));
    UmcResult result = {19, 23U};
    size_t used = 99U;
    size_t count = 4U, capacity = 4U;
    UmcStatus expected = UMC_BAD_BYTECODE;
    const UmcInstruction *input = program;
    UmcResult *output = &result;
    UmcTraceStep *storage = steps;
    size_t *written = &used;
    if (strcmp(name, "short-capacity") == 0) { capacity = 3U; expected = UMC_BUFFER_TOO_SMALL; }
    else if (strcmp(name, "zero-capacity") == 0) { capacity = 0U; expected = UMC_BUFFER_TOO_SMALL; }
    else if (strcmp(name, "bad-opcode") == 0) program[2].opcode = (UmcOpcode)255;
    else if (strcmp(name, "reserved-operand") == 0) program[2].operand = 1;
    else if (strcmp(name, "bad-source-offset") == 0) program[2].source_offset = UMC_SOURCE_LIMIT + 1U;
    else if (strcmp(name, "early-return") == 0) { program[1].opcode = UMC_RETURN; program[1].operand = 0; }
    else if (strcmp(name, "missing-return") == 0) count = 3U;
    else if (strcmp(name, "null-program") == 0) input = NULL;
    else if (strcmp(name, "zero-count") == 0) count = 0U;
    else if (strcmp(name, "excess-count") == 0) count = UMC_PROGRAM_LIMIT + 1U;
    else if (strcmp(name, "null-result") == 0) { output = NULL; expected = UMC_INVALID_ARGUMENT; }
    else if (strcmp(name, "null-storage") == 0) { storage = NULL; expected = UMC_INVALID_ARGUMENT; }
    else if (strcmp(name, "null-count") == 0) { written = NULL; expected = UMC_INVALID_ARGUMENT; }
    else CHECK(0);
    /* A zero budget must not hide a malformed suffix or insufficient storage. */
    CHECK(umc_execute_trace(input, count, 0U, output, storage, capacity, written, NULL) == expected);
    CHECK(used == (written == NULL ? 99U : 0U));
    CHECK(result.value == 19 && result.instructions_executed == 23U);
    CHECK(memcmp(before, steps, sizeof(before)) == 0);
    return 0;
}

static int reuse(void)
{
    UmcTraceStep steps[4];
    UmcResult result = {19, 23U};
    size_t used = 0U;
    CHECK(umc_execute_trace(good, 4U, 1U, &result, steps, 4U, &used, NULL) == UMC_FUEL_EXHAUSTED);
    CHECK(used == 2U);
    CHECK(umc_execute_trace(good, 4U, 4U, &result, steps, 4U, &used, NULL) == UMC_OK);
    CHECK(used == 4U && steps[1].status == UMC_OK && result.value == 42);
    CHECK(umc_execute_trace(good, 4U, 0U, &result, steps, 4U, &used, NULL) == UMC_FUEL_EXHAUSTED);
    CHECK(used == 1U && result.value == 42 && result.instructions_executed == 4U);
    return 0;
}

static int wire(void)
{
    uint8_t bytes[UMC_BYTECODE_LIMIT];
    size_t length = 0U, count = 0U, used = 0U;
    UmcInstruction decoded[4];
    UmcTraceStep steps[4];
    UmcResult result;
    CHECK(umc_encode(good, 4U, bytes, sizeof(bytes), &length, NULL) == UMC_OK);
    CHECK(umc_decode(bytes, length, decoded, 4U, &count, NULL) == UMC_OK);
    CHECK(count == 4U && umc_execute_trace(decoded, count, count, &result, steps, 4U, &used, NULL) == UMC_OK);
    CHECK(used == count && result.value == 42 && steps[2].source_offset == 11U);
    return 0;
}

/* Exercise the public maximums independently of the four-instruction fixture. */
static int limits(const char *name)
{
    UmcInstruction program[UMC_PROGRAM_LIMIT];
    UmcTraceStep steps[UMC_PROGRAM_LIMIT];
    UmcResult result = {19, 23U};
    size_t count = 0U, used = 99U;
    if (strcmp(name, "max-program") == 0) {
        program[count++] = (UmcInstruction){UMC_PUSH, 1, 0U};
        while (count + 1U < UMC_PROGRAM_LIMIT) {
            program[count] = (UmcInstruction){UMC_NEGATE, 0, count};
            ++count;
        }
        program[count++] = (UmcInstruction){UMC_RETURN, 0, 0U};
        CHECK(umc_execute_trace(program, count, count, &result, steps,
            UMC_PROGRAM_LIMIT, &used, NULL) == UMC_OK);
        CHECK(count == UMC_PROGRAM_LIMIT && used == count && result.instructions_executed == count);
        CHECK(result.value == 1 && steps[count - 1U].opcode == UMC_RETURN);
        return 0;
    }
    const int overflow = strcmp(name, "stack-overflow") == 0;
    const size_t pushes = UMC_STACK_LIMIT + (overflow ? 1U : 0U);
    for (size_t i = 0U; i < pushes; ++i)
        program[count++] = (UmcInstruction){UMC_PUSH, 1, i};
    for (size_t i = 1U; i < pushes; ++i)
        program[count++] = (UmcInstruction){UMC_ADD, 0, i};
    program[count++] = (UmcInstruction){UMC_RETURN, 0, 0U};
    memset(steps, 0x5a, sizeof steps);
    unsigned char before[sizeof steps];
    memcpy(before, steps, sizeof before);
    const UmcStatus status = umc_execute_trace(program, count, count, &result,
        steps, UMC_PROGRAM_LIMIT, &used, NULL);
    if (overflow) {
        CHECK(status == UMC_STACK_OVERFLOW && used == 0U);
        CHECK(result.value == 19 && result.instructions_executed == 23U);
        CHECK(memcmp(before, steps, sizeof before) == 0);
    } else {
        CHECK(status == UMC_OK && used == count && result.value == (int32_t)UMC_STACK_LIMIT);
        CHECK(steps[UMC_STACK_LIMIT - 1U].stack_depth == UMC_STACK_LIMIT);
        CHECK(steps[count - 1U].stack_depth == 1U && steps[count - 1U].top == (int32_t)UMC_STACK_LIMIT);
    }
    return 0;
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    if (strcmp(argv[1], "max-program") == 0 || strcmp(argv[1], "max-depth") == 0 ||
        strcmp(argv[1], "stack-overflow") == 0) return limits(argv[1]);
    if (strcmp(argv[1], "success") == 0) return success();
    if (strcmp(argv[1], "fuel-boundaries") == 0) return fuel_boundaries();
    if (strcmp(argv[1], "unary") == 0) return unary();
    if (strcmp(argv[1], "reuse") == 0) return reuse();
    if (strcmp(argv[1], "wire") == 0) return wire();
    if (strstr(argv[1], "overflow") != NULL || strcmp(argv[1], "divide-zero") == 0 ||
        strcmp(argv[1], "negative-division") == 0 || strcmp(argv[1], "minimum-add") == 0 ||
        strcmp(argv[1], "maximum-subtract") == 0) return arithmetic(argv[1]);
    return refused(argv[1]);
}
