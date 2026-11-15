/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_runtime_profile.c
 *
 * PURPOSE:
 *   Exercise runtime-only capability discovery, wire formats, inspection and tracing.
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
#include "umicom_c/trace.h"

int main(void)
{
    const uint32_t features = umc_runtime_profile(UMC_PROFILE_FEATURES);
    const uint32_t formats = umc_runtime_profile(UMC_PROFILE_WIRE_VERSIONS);
    CHECK(umc_runtime_profile(UMC_PROFILE_LOCAL_VALUES) == UMC_LOCAL_LIMIT);
    CHECK(umc_runtime_profile(UMC_PROFILE_IDENTIFIER_BYTES) == 0U);
    CHECK(features & UMC_FEATURE_LOCAL_INTEGERS);
    CHECK(features & UMC_FEATURE_FORWARD_BRANCHES);
    CHECK(features & UMC_FEATURE_REMAINDER);
    CHECK(features & UMC_FEATURE_BINARY_BITWISE);
    CHECK(formats & (UINT32_C(1) << 6U));
    CHECK(formats & (UINT32_C(1) << 5U));
    CHECK(!(features & UMC_FEATURE_LOGICAL_CONNECTIVES));
    CHECK(!(features & UMC_FEATURE_CONDITIONAL_EXPRESSIONS));
    CHECK(!(features & UMC_FEATURE_COMPOUND_ASSIGNMENTS));
    CHECK(!(features & UMC_FEATURE_IF_STATEMENTS));
    CHECK(!(features & UMC_FEATURE_ELSE_IF_CHAINS));
    CHECK(formats & (UINT32_C(1) << 4U));
    CHECK(umc_runtime_profile(UMC_PROFILE_API) == UMC_API_VERSION);
    CHECK(umc_runtime_profile(UMC_PROFILE_PROGRAM_INSTRUCTIONS) == UMC_PROGRAM_LIMIT);
    CHECK(umc_runtime_profile(UMC_PROFILE_STACK_VALUES) == UMC_STACK_LIMIT);
    CHECK(umc_runtime_profile(UMC_PROFILE_BYTECODE_BYTES) == UMC_BYTECODE_LIMIT);
    CHECK(umc_runtime_profile(UMC_PROFILE_SOURCE_BYTES) == 0U);
    CHECK(umc_runtime_profile(UMC_PROFILE_PARSE_DEPTH) == 0U);
    CHECK(umc_runtime_profile((UmcProfileProperty)0) == 0U);
    CHECK(umc_runtime_profile((UmcProfileProperty)999) == 0U);
    CHECK(!(features & UMC_FEATURE_LOGICAL_NOT));
    CHECK(!(features & (UMC_FEATURE_HEXADECIMAL_LITERALS | UMC_FEATURE_BINARY_LITERALS)));
    CHECK((features & (UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON |
        UMC_FEATURE_EXECUTION_TRACE | UMC_FEATURE_PROGRAM_INSPECTION)) ==
        (UMC_FEATURE_ARITHMETIC | UMC_FEATURE_COMPARISON |
        UMC_FEATURE_EXECUTION_TRACE | UMC_FEATURE_PROGRAM_INSPECTION));
    /* No compiler is linked. Fixed programs independently exercise each
     * advertised wire profile and the runtime operations used by clients. */
    const UmcInstruction programs[2][4] = {
        {{UMC_PUSH, 7, 0U}, {UMC_PUSH, 6, 2U}, {UMC_MULTIPLY, 0, 1U}, {UMC_RETURN, 0, 3U}},
        {{UMC_PUSH, 7, 0U}, {UMC_PUSH, 6, 2U}, {UMC_GREATER, 0, 1U}, {UMC_RETURN, 0, 3U}}
    };
    for (size_t i = 0U; i < 2U; ++i) {
        uint8_t wire[UMC_BYTECODE_LIMIT];
        UmcInstruction decoded[4]; UmcDiagnostic diagnostic;
        UmcResult result; UmcProgramInfo info; UmcTraceStep steps[4];
        size_t bytes = 0U, count = 0U, used = 0U;
        CHECK(umc_encode(programs[i], 4U, wire, sizeof wire, &bytes, &diagnostic) == UMC_OK);
        CHECK(wire[4] == i + 1U);
        CHECK(formats & (UINT32_C(1) << wire[4]));
        CHECK(umc_decode(wire, bytes, decoded, 4U, &count, &diagnostic) == UMC_OK);
        CHECK(umc_inspect(decoded, count, &info, &diagnostic) == UMC_OK);
        CHECK(info.instruction_count == 4U && info.peak_stack == 2U);
        CHECK(umc_execute_trace(decoded, count, info.instruction_count, &result,
            steps, 4U, &used, &diagnostic) == UMC_OK);
        CHECK(result.value == (i ? 1 : 42) && used == 4U);
        wire[4] = 3U; count = 9U;
        /* Format 3 is supported for locals, but noncanonical for these programs.
         * Retain the former unsupported-profile assertion for review. */
#if 0
        CHECK(!(formats & (UINT32_C(1) << 3U)));
#endif
        CHECK(formats & (UINT32_C(1) << 3U));
        CHECK(umc_decode(wire, bytes, decoded, 4U, &count, &diagnostic) == UMC_BAD_BYTECODE);
        CHECK(count == 0U);
    }
    /* Runtime-only linkage verifies the advertised operation without a parser. */
    const UmcInstruction remainder[]={{UMC_PUSH,-7,0U},{UMC_PUSH,3,1U},
        {UMC_REMAINDER,0,2U},{UMC_RETURN,0,3U}};
    uint8_t remainder_wire[UMC_BYTECODE_LIMIT];size_t remainder_size=0U;
    UmcDiagnostic remainder_diagnostic;UmcResult remainder_result;
    CHECK(umc_encode(remainder,4U,remainder_wire,sizeof remainder_wire,
        &remainder_size,&remainder_diagnostic)==UMC_OK && remainder_wire[4]==5U);
    CHECK(umc_execute(remainder,4U,4U,&remainder_result,&remainder_diagnostic)==UMC_OK &&
        remainder_result.value==-1);
    /* Runtime-only linkage must execute each advertised bitwise identity. */
    const int32_t bitwise_expected[] = {42, -43, -1};
    for (unsigned i = 0U; i < 3U; ++i) {
        UmcInstruction bitwise[] = {{UMC_PUSH,-1,0U},{UMC_PUSH,42,1U},
            {(UmcOpcode)((unsigned)UMC_BITWISE_AND+i),0,2U},{UMC_RETURN,0,3U}};
        uint8_t bytes[44]; size_t written=0U; UmcDiagnostic d; UmcResult r;
        CHECK(umc_encode(bitwise,4U,bytes,sizeof bytes,&written,&d)==UMC_OK);
        CHECK(bytes[4]==6U && written==sizeof bytes);
        CHECK(umc_execute(bitwise,4U,4U,&r,&d)==UMC_OK && r.value==bitwise_expected[i]);
    }
    return 0;
}
