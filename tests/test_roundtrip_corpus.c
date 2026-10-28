/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_roundtrip_corpus.c
 *
 * PURPOSE:
 *   Compare source, wire and native arithmetic results using a reproducible corpus.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "test.h"
#include "umicom_c/comparison.h"
#include <stdint.h>
/* A fixed seed makes failures reproducible. Small values keep the independent
 * C oracle away from signed overflow; explicit limit cases live in test_vm.c. */
int main(void)
{
    uint32_t state = 0x6d2b79f5U;
    for (size_t case_id = 0U; case_id < 512U; ++case_id) {
        state = state * 1664525U + 1013904223U;
        const int a = (int)(state % 91U) + 1;
        state = state * 1664525U + 1013904223U;
        const int b = (int)(state % 31U) + 1;
        state = state * 1664525U + 1013904223U;
        const int c = (int)(state % 17U) + 1;
        char source[160];
        const int length = snprintf(source, sizeof source,
            "int main(void) { return ((%d * %d) - %d) / %d; }", a, b, c, c);
        CHECK(length > 0 && (size_t)length < sizeof source);
        UmcInstruction original[UMC_PROGRAM_LIMIT], decoded[UMC_PROGRAM_LIMIT];
        uint8_t wire[UMC_BYTECODE_LIMIT];
        size_t count = 0U, decoded_count = 0U, bytes = 0U;
        UmcDiagnostic diagnostic;
        UmcResult direct, restored;
        CHECK(umc_compile(source, (size_t)length, original, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
        CHECK(umc_execute(original, count, count, &direct, &diagnostic) == UMC_OK);
        CHECK(direct.value == ((a * b) - c) / c);
        CHECK(umc_encode(original, count, wire, sizeof wire, &bytes, &diagnostic) == UMC_OK);
        CHECK(umc_decode(wire, bytes, decoded, UMC_PROGRAM_LIMIT, &decoded_count, &diagnostic) == UMC_OK);
        CHECK(umc_execute(decoded, decoded_count, decoded_count, &restored, &diagnostic) == UMC_OK);
        CHECK(restored.value == direct.value && restored.instructions_executed == direct.instructions_executed);
        bool same = false;
        CHECK(umc_program_same_operations(original, count, decoded, decoded_count, &same, &diagnostic) == UMC_OK && same);
        restored.value = 123;
        restored.instructions_executed = 456U;
        CHECK(umc_execute(decoded, decoded_count, decoded_count - 1U, &restored, &diagnostic) == UMC_FUEL_EXHAUSTED);
        CHECK(restored.value == 123 && restored.instructions_executed == 456U);
    }
    return 0;
}
