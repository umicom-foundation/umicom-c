/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_vm.c
 *
 * PURPOSE:
 *   Reject malformed programs and arithmetic hazards before publishing a result.
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
    static const UmcInstruction good[] = {
        {UMC_PUSH, 40, 0U}, {UMC_PUSH, 2, 3U}, {UMC_ADD, 0, 2U}, {UMC_RETURN, 0, 0U}
    };
    UmcDiagnostic diagnostic;
    UmcResult result = {123, 456U};
    CHECK(umc_execute(good, 4U, 4U, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == 42 && result.instructions_executed == 4U);
    for (size_t fuel = 0U; fuel < 4U; ++fuel) {
        result.value = 123; result.instructions_executed = 456U;
        CHECK(umc_execute(good, 4U, fuel, &result, &diagnostic) == UMC_FUEL_EXHAUSTED);
        CHECK(result.value == 123 && result.instructions_executed == 456U);
    }
    static const struct { int32_t left; int32_t right; UmcOpcode opcode; } hazards[] = {
        {INT32_MAX, 1, UMC_ADD}, {INT32_MIN, 1, UMC_SUBTRACT},
        {INT32_MAX, 2, UMC_MULTIPLY}, {INT32_MIN, -1, UMC_DIVIDE},
        {4, 0, UMC_DIVIDE}
    };
    for (size_t i = 0U; i < sizeof(hazards) / sizeof(hazards[0]); ++i) {
        UmcInstruction program[] = {
            {UMC_PUSH, hazards[i].left, 0U}, {UMC_PUSH, hazards[i].right, 0U},
            {hazards[i].opcode, 0, 10U}, {UMC_RETURN, 0, 0U}
        };
        result.value = 123; result.instructions_executed = 456U;
        CHECK(umc_execute(program, 4U, 4U, &result, &diagnostic) == UMC_ARITHMETIC_ERROR);
        CHECK(result.value == 123 && result.instructions_executed == 456U);
        CHECK(diagnostic.source_offset == 10U);
    }
    static const UmcInstruction negative[] = {
        {UMC_PUSH, INT32_MIN, 0U}, {UMC_NEGATE, 0, 1U}, {UMC_RETURN, 0, 0U}
    };
    CHECK(umc_execute(negative, 3U, 3U, &result, NULL) == UMC_ARITHMETIC_ERROR);
    static const UmcInstruction underflow[] = {{UMC_ADD, 0, 0U}, {UMC_RETURN, 0, 0U}};
    CHECK(umc_validate(underflow, 2U, NULL) == UMC_BAD_BYTECODE);
    static const UmcInstruction suffix[] = {
        {UMC_PUSH, 1, 0U}, {UMC_RETURN, 0, 0U}, {UMC_PUSH, 9, 0U}
    };
    CHECK(umc_execute(suffix, 3U, 0U, &result, NULL) == UMC_BAD_BYTECODE);
    static const UmcInstruction reserved[] = {{UMC_PUSH, 1, 0U}, {UMC_RETURN, 9, 0U}};
    CHECK(umc_validate(reserved, 2U, NULL) == UMC_BAD_BYTECODE);
    static const UmcInstruction unknown[] = {{(UmcOpcode)255, 0, 0U}};
    CHECK(umc_validate(unknown, 1U, NULL) == UMC_BAD_BYTECODE);
    CHECK(umc_validate(good, 3U, NULL) == UMC_BAD_BYTECODE);
    CHECK(umc_validate(good, UMC_PROGRAM_LIMIT + 1U, NULL) == UMC_BAD_BYTECODE);
    CHECK(umc_validate(NULL, 0U, NULL) == UMC_BAD_BYTECODE);
    UmcInstruction deep[UMC_STACK_LIMIT + 1U];
    for (size_t i = 0U; i < UMC_STACK_LIMIT + 1U; ++i) {
        deep[i].opcode = UMC_PUSH; deep[i].operand = 1; deep[i].source_offset = 0U;
    }
    CHECK(umc_validate(deep, UMC_STACK_LIMIT + 1U, NULL) == UMC_STACK_OVERFLOW);
    return 0;
}
