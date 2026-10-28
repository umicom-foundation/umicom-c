/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_wire_contract.c
 *
 * PURPOSE:
 *   Verify fixed bytecode fixtures independently of compiler output and host alignment.
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

/* A fixed fixture catches an encoder and decoder drifting together. The byte
 * positions are the published wire contract, not sizeof(UmcInstruction). */
static const uint8_t smallest_integer[] = {
    0x55, 0x4d, 0x43, 0x42, 1, 0, 2, 0,
    1, 0, 0, 0, 0x80, 0, 0, 1, 0,
    7, 0, 0, 0, 0, 0, 0, 0, 0
};
static const uint8_t arithmetic[] = {
    0x55, 0x4d, 0x43, 0x42, 1, 0, 4, 0,
    1, 7, 0, 0, 0, 0, 0, 0, 0,
    1, 6, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0,
    7, 0, 0, 0, 0, 0, 0, 0, 0
};

int main(void)
{
    UmcInstruction decoded[4];
    UmcResult result;
    UmcDiagnostic diagnostic;
    uint8_t encoded[sizeof arithmetic];
    /* Align the array, then deliberately offset the input by one byte. A
     * decoder must read bytes rather than cast to an aligned integer pointer. */
    _Alignas(uint64_t) uint8_t unaligned[sizeof arithmetic + 1U];
    size_t count = 0U, length = 0U;
    CHECK(umc_decode(smallest_integer, sizeof smallest_integer, decoded, 4U,
        &count, &diagnostic) == UMC_OK);
    CHECK(count == 2U && decoded[0].operand == INT32_MIN);
    CHECK(decoded[0].source_offset == 65536U);
    CHECK(umc_execute(decoded, count, 2U, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == INT32_MIN && result.instructions_executed == 2U);
    CHECK(umc_encode(decoded, count, encoded, sizeof encoded, &length, NULL) == UMC_OK);
    CHECK(length == sizeof smallest_integer);
    CHECK(memcmp(encoded, smallest_integer, length) == 0);

    memcpy(unaligned + 1U, arithmetic, sizeof arithmetic);
    CHECK(umc_decode(unaligned + 1U, sizeof arithmetic, decoded, 4U,
        &count, &diagnostic) == UMC_OK);
    CHECK(umc_execute(decoded, count, 4U, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == 42 && result.instructions_executed == 4U);
    CHECK(umc_encode(decoded, count, encoded, sizeof encoded, &length, NULL) == UMC_OK);
    CHECK(length == sizeof arithmetic && memcmp(encoded, arithmetic, length) == 0);

    /* Reverse only the two count bytes to simulate a wrong-endian producer.
     * This represents 1024 records, not four, and must not be accepted. */
    unaligned[7] = 0U;
    unaligned[8] = 4U;
    count = 99U;
    CHECK(umc_decode(unaligned + 1U, sizeof arithmetic, decoded, 4U,
        &count, NULL) == UMC_BAD_BYTECODE);
    CHECK(count == 0U);
    return 0;
}
