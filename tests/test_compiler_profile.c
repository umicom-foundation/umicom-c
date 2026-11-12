/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_compiler_profile.c
 *
 * PURPOSE:
 *   Check reported source features against actual compilation and runtime behavior.
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

int main(void)
{
    const uint32_t features = umc_compiler_profile(UMC_PROFILE_FEATURES);
    static const struct { uint32_t feature; const char *source; int32_t value; } cases[] = {
        {UMC_FEATURE_ARITHMETIC, "int main(void){return 2+3*4;}", 14},
        {UMC_FEATURE_COMPARISON, "int main(void){return 2+3<6;}", 1},
        {UMC_FEATURE_LOGICAL_NOT, "int main(void){return !0;}", 1},
        {UMC_FEATURE_HEXADECIMAL_LITERALS, "int main(void){return 0x2A;}", 42},
        {UMC_FEATURE_BINARY_LITERALS, "int main(void){return 0b101010;}", 42},
        {UMC_FEATURE_LOCAL_INTEGERS, "int main(void){int x=6; x=x*7; return x;}", 42},
        {UMC_FEATURE_LOGICAL_CONNECTIVES, "int main(void){int x=0;return x && (1/x);}", 0},
        {UMC_FEATURE_CONDITIONAL_EXPRESSIONS, "int main(void){int x=0;return x ? 1/x : 42;}", 42}
    };
    CHECK(umc_compiler_profile(UMC_PROFILE_LOCAL_VALUES) == UMC_LOCAL_LIMIT);
    CHECK(umc_compiler_profile(UMC_PROFILE_IDENTIFIER_BYTES) == UMC_IDENTIFIER_LIMIT);
    CHECK(umc_compiler_profile(UMC_PROFILE_API) == UMC_API_VERSION);
    CHECK(umc_compiler_profile(UMC_PROFILE_PROGRAM_INSTRUCTIONS) == UMC_PROGRAM_LIMIT);
    CHECK(umc_compiler_profile(UMC_PROFILE_SOURCE_BYTES) == UMC_SOURCE_LIMIT);
    CHECK(umc_compiler_profile(UMC_PROFILE_PARSE_DEPTH) == UMC_PARSE_DEPTH_LIMIT);
    CHECK(!(features & (UMC_FEATURE_EXECUTION_TRACE | UMC_FEATURE_PROGRAM_INSPECTION | UMC_FEATURE_FORWARD_BRANCHES)));
    CHECK(umc_compiler_profile(UMC_PROFILE_WIRE_VERSIONS) == 0U);
    CHECK(umc_compiler_profile(UMC_PROFILE_STACK_VALUES) == 0U);
    CHECK(umc_compiler_profile((UmcProfileProperty)0) == 0U);
    CHECK(umc_compiler_profile((UmcProfileProperty)999) == 0U);
    for (size_t i = 0U; i < sizeof cases / sizeof cases[0]; ++i) {
        UmcInstruction program[UMC_PROGRAM_LIMIT];
        UmcDiagnostic diagnostic; UmcResult result; size_t count = 0U;
        CHECK(features & cases[i].feature);
        CHECK(umc_compile(cases[i].source, strlen(cases[i].source), program,
            umc_compiler_profile(UMC_PROFILE_PROGRAM_INSTRUCTIONS), &count, &diagnostic) == UMC_OK);
        CHECK(umc_execute(program, count, count, &result, &diagnostic) == UMC_OK);
        CHECK(result.value == cases[i].value);
    }
    /* Bounds are operational: a reported source limit does not authorize
     * reading any bytes beyond a caller-owned span. The oversized length is
     * rejected before this short literal can be scanned. */
    UmcInstruction program[8]; UmcDiagnostic diagnostic; size_t count = 73U;
    CHECK(umc_compile("x", (size_t)umc_compiler_profile(UMC_PROFILE_SOURCE_BYTES) + 1U,
        program, 8U, &count, &diagnostic) == UMC_SOURCE_TOO_LARGE);
    CHECK(count == 0U);
    /* Paired logical operators now compile; single bitwise operators remain
     * outside the source profile. Retain the earlier fixture for review. */
#if 0
    const char unsupported[] = "int main(void){return 1 && 1;}";
#endif
    const char unsupported[] = "int main(void){return 1 & 1;}";
    CHECK(umc_compile(unsupported, sizeof unsupported - 1U, program, 8U,
        &count, &diagnostic) == UMC_SYNTAX_ERROR);
    return 0;
}
