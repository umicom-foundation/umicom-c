/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_bytecode.c
 *
 * PURPOSE:
 *   Verify portable encoding, exact length checks and hostile artifact rejection.
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
    static const UmcInstruction original[] = {{UMC_PUSH, INT32_MIN, 10U}, {UMC_RETURN, 0, 0U}};
    uint8_t bytes[UMC_BYTECODE_LIMIT];
    UmcInstruction decoded[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    size_t length = 999U, count = 999U;
    CHECK(umc_encode(original, 2U, bytes, sizeof(bytes), &length, &diagnostic) == UMC_OK);
    CHECK(length == 26U && bytes[6] == 2U && bytes[7] == 0U);
    CHECK(bytes[9] == 0U && bytes[12] == 128U); /* Explicit little-endian INT32_MIN. */
    CHECK(umc_decode(bytes, length, decoded, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    CHECK(count == 2U && decoded[0].operand == INT32_MIN && decoded[0].source_offset == 10U);
    UmcResult result;
    CHECK(umc_execute(decoded, count, count, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == INT32_MIN);
    for (size_t truncated = 0U; truncated < length; ++truncated) {
        count = 999U;
        CHECK(umc_decode(bytes, truncated, decoded, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_BAD_BYTECODE);
        CHECK(count == 0U);
    }
    bytes[length] = 0U;
    CHECK(umc_decode(bytes, length + 1U, decoded, UMC_PROGRAM_LIMIT, &count, NULL) == UMC_BAD_BYTECODE);
    bytes[4] = 2U;
    CHECK(umc_decode(bytes, length, decoded, UMC_PROGRAM_LIMIT, &count, NULL) == UMC_BAD_BYTECODE);
    bytes[4] = 1U; bytes[5] = 1U;
    CHECK(umc_decode(bytes, length, decoded, UMC_PROGRAM_LIMIT, &count, NULL) == UMC_BAD_BYTECODE);
    bytes[5] = 0U; bytes[8] = 255U;
    CHECK(umc_decode(bytes, length, decoded, UMC_PROGRAM_LIMIT, &count, NULL) == UMC_BAD_BYTECODE);
    CHECK(count == 0U);
    CHECK(umc_encode(original, 2U, bytes, sizeof(bytes), &length, NULL) == UMC_OK);
    CHECK(umc_decode(bytes, length, decoded, 1U, &count, NULL) == UMC_BUFFER_TOO_SMALL);
    CHECK(umc_encode(original, 2U, bytes, 1U, &length, NULL) == UMC_BUFFER_TOO_SMALL);
    CHECK(length == 0U);
    return 0;
}
