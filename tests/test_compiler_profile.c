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
        {UMC_FEATURE_COMPOUND_STATEMENTS, "int main(void){int r=0;{int v=42;r=v;}return r;}", 42},
        {UMC_FEATURE_BRANCH_LOCALS, "int main(void){int result=0;if(1){const int x=42;result=x;}return result;}", 42},
        {UMC_FEATURE_BOOLEAN_LITERALS, "int main(void){return true ? 42 : false;}", 42},
        {UMC_FEATURE_DECLARATOR_LISTS, "int main(void){int a=6,b=a*7;return b;}", 42},
        {UMC_FEATURE_ASCII_CHARACTER_LITERALS, "int main(void){return '*';}", 42},
        {UMC_FEATURE_EMPTY_STATEMENTS, "int main(void){;{;;};return 42;;}", 42},
        {UMC_FEATURE_CHECKED_SHIFTS, "int main(void){return 21<<1;}", 42},
        {UMC_FEATURE_READONLY_LOCALS, "int main(void){const int x=42;return x;}", 42},
        {UMC_FEATURE_DIGIT_SEPARATORS, "int main(void){return 0x2'A;}", 42},
        {UMC_FEATURE_INCREMENT_STATEMENTS, "int main(void){int x=41;++x;return x;}", 42},
        {UMC_FEATURE_BINARY_BITWISE, "int main(void){return 63&42;}", 42},
        {UMC_FEATURE_BITWISE_COMPLEMENT, "int main(void){return ~(-2147483647-1);}", INT32_MAX},
        {UMC_FEATURE_REMAINDER, "int main(void){int x=137;x%=95;return x;}", 42},
        {UMC_FEATURE_ARITHMETIC, "int main(void){return 2+3*4;}", 14},
        {UMC_FEATURE_COMPARISON, "int main(void){return 2+3<6;}", 1},
        {UMC_FEATURE_LOGICAL_NOT, "int main(void){return !0;}", 1},
        {UMC_FEATURE_HEXADECIMAL_LITERALS, "int main(void){return 0x2A;}", 42},
        {UMC_FEATURE_BINARY_LITERALS, "int main(void){return 0b101010;}", 42},
        {UMC_FEATURE_LOCAL_INTEGERS, "int main(void){int x=6; x=x*7; return x;}", 42},
        {UMC_FEATURE_LOGICAL_CONNECTIVES, "int main(void){int x=0;return x && (1/x);}", 0},
        {UMC_FEATURE_CONDITIONAL_EXPRESSIONS, "int main(void){int x=0;return x ? 1/x : 42;}", 42},
        {UMC_FEATURE_COMPOUND_ASSIGNMENTS, "int main(void){int x=6;x*=7;return x;}", 42},
        {UMC_FEATURE_ELSE_IF_CHAINS, "int main(void){int x=1;if(0){x=7;}else if(1){x=42;}return x;}", 42},
        {UMC_FEATURE_IF_STATEMENTS, "int main(void){int x=1;if(0){x=7;}else{x=42;}return x;}", 42}
    };
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES) & UMC_FEATURE_BITWISE_COMPLEMENT));
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
/* Binary bitwise is supported; retain the earlier refusal and keep shifts outside the profile. */
#if 0
    const char unsupported[] = "int main(void){return 1 & 1;}";
#endif
/* Keep unsupported-token coverage while shifts become a declared compiler capability. The former implementation is retained for owner review. */
#if 0
    const char unsupported[] = "int main(void){return 1 << 1;}";
#endif
    const char unsupported[] = "int main(void){return 1 <<< 1;}";
    CHECK(umc_compile(unsupported, sizeof unsupported - 1U, program, 8U,
        &count, &diagnostic) == UMC_SYNTAX_ERROR);
    return 0;
}
