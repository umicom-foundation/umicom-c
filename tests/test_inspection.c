/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_inspection.c
 *
 * PURPOSE:
 *   Verify structural resource counts, failure atomicity and execution-policy boundaries.
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

static int unchanged(const UmcProgramInfo *info)
{
    return info->instruction_count == 91U && info->peak_stack == 92U &&
        info->constant_count == 93U && info->arithmetic_count == 94U &&
        info->bytecode_size == 95U;
}

int main(void)
{
    static const UmcInstruction expression[] = {
        {UMC_PUSH, 2, 0U}, {UMC_PUSH, 3, 1U}, {UMC_PUSH, 4, 2U},
        {UMC_MULTIPLY, 0, 3U}, {UMC_ADD, 0, 4U},
        {UMC_NEGATE, 0, 5U}, {UMC_RETURN, 0, 6U}
    };
    UmcProgramInfo info;
    UmcDiagnostic diagnostic;
    CHECK(umc_inspect(expression, 7U, &info, &diagnostic) == UMC_OK);
    CHECK(info.instruction_count == 7U && info.peak_stack == 3U);
    CHECK(info.constant_count == 3U && info.arithmetic_count == 3U);
    CHECK(info.bytecode_size == 71U);
    uint8_t wire[UMC_BYTECODE_LIMIT];
    size_t bytes = 0U;
    CHECK(umc_encode(expression, 7U, wire, sizeof(wire), &bytes, NULL) == UMC_OK);
    CHECK(bytes == info.bytecode_size);
    UmcResult result = {99, 99U};
    CHECK(umc_execute(expression, 7U, info.instruction_count - 1U, &result, NULL) == UMC_FUEL_EXHAUSTED);
    CHECK(result.value == 99 && result.instructions_executed == 99U);
    CHECK(umc_execute(expression, 7U, info.instruction_count, &result, NULL) == UMC_OK);
    CHECK(result.value == -14 && result.instructions_executed == 7U);

    /* Structural acceptance must not be reported as successful arithmetic. */
    static const UmcInstruction zero_divisor[] = {
        {UMC_PUSH, 1, 0U}, {UMC_PUSH, 0, 0U}, {UMC_DIVIDE, 0, 8U}, {UMC_RETURN, 0, 0U}
    };
    CHECK(umc_inspect(zero_divisor, 4U, &info, NULL) == UMC_OK);
    CHECK(umc_execute(zero_divisor, 4U, info.instruction_count, &result, NULL) == UMC_ARITHMETIC_ERROR);

    static const UmcInstruction invalid[][3] = {
        {{UMC_PUSH, 1, 0U}, {UMC_RETURN, 0, 6U}, {UMC_NEGATE, 0, 0U}},
        {{UMC_PUSH, 1, 0U}, {UMC_NEGATE, 1, 6U}, {UMC_RETURN, 0, 0U}},
        {{UMC_PUSH, 1, 0U}, {(UmcOpcode)255, 0, 6U}, {UMC_RETURN, 0, 0U}},
        {{UMC_PUSH, 1, 0U}, {UMC_ADD, 0, 6U}, {UMC_RETURN, 0, 0U}},
        {{UMC_PUSH, 1, 0U}, {UMC_NEGATE, 0, UMC_SOURCE_LIMIT + 1U}, {UMC_RETURN, 0, 0U}}
    };
    for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        info = (UmcProgramInfo){91U, 92U, 93U, 94U, 95U};
        CHECK(umc_inspect(invalid[i], 3U, &info, &diagnostic) == UMC_BAD_BYTECODE);
        CHECK(unchanged(&info));
        CHECK(diagnostic.source_offset == invalid[i][1].source_offset);
        CHECK(umc_validate(invalid[i], 3U, NULL) == UMC_BAD_BYTECODE);
    }
    CHECK(umc_inspect(NULL, 1U, &info, NULL) == UMC_BAD_BYTECODE);
    CHECK(unchanged(&info));
    CHECK(umc_inspect(expression, 0U, &info, NULL) == UMC_BAD_BYTECODE);
    CHECK(unchanged(&info));
    CHECK(umc_inspect(expression, UMC_PROGRAM_LIMIT + 1U, &info, NULL) == UMC_BAD_BYTECODE);
    CHECK(unchanged(&info));
    CHECK(umc_inspect(expression, 7U, NULL, &diagnostic) == UMC_INVALID_ARGUMENT);

    UmcInstruction boundary[UMC_PROGRAM_LIMIT];
    size_t count = 0U;
    for (size_t i = 0U; i < UMC_STACK_LIMIT; ++i)
        boundary[count++] = (UmcInstruction){UMC_PUSH, 1, 0U};
    for (size_t i = 1U; i < UMC_STACK_LIMIT; ++i)
        boundary[count++] = (UmcInstruction){UMC_ADD, 0, 0U};
    boundary[count++] = (UmcInstruction){UMC_RETURN, 0, 0U};
    CHECK(umc_inspect(boundary, count, &info, NULL) == UMC_OK);
    CHECK(info.peak_stack == UMC_STACK_LIMIT && info.constant_count == UMC_STACK_LIMIT);
    CHECK(info.arithmetic_count == UMC_STACK_LIMIT - 1U);
    CHECK(umc_execute(boundary, count, count, &result, NULL) == UMC_OK);
    CHECK(result.value == (int32_t)UMC_STACK_LIMIT);
    boundary[UMC_STACK_LIMIT] = (UmcInstruction){UMC_PUSH, 1, 0U};
    info = (UmcProgramInfo){91U, 92U, 93U, 94U, 95U};
    CHECK(umc_inspect(boundary, count, &info, NULL) == UMC_STACK_OVERFLOW);
    CHECK(unchanged(&info));

    boundary[0] = (UmcInstruction){UMC_PUSH, 1, 0U};
    for (size_t i = 1U; i + 1U < UMC_PROGRAM_LIMIT; ++i)
        boundary[i] = (UmcInstruction){UMC_NEGATE, 0, 0U};
    boundary[UMC_PROGRAM_LIMIT - 1U] = (UmcInstruction){UMC_RETURN, 0, 0U};
    CHECK(umc_inspect(boundary, UMC_PROGRAM_LIMIT, &info, NULL) == UMC_OK);
    CHECK(info.instruction_count == UMC_PROGRAM_LIMIT && info.peak_stack == 1U);
    CHECK(info.bytecode_size == UMC_BYTECODE_LIMIT);
    CHECK(info.arithmetic_count == UMC_PROGRAM_LIMIT - 2U && info.constant_count == 1U);
    return 0;
}
