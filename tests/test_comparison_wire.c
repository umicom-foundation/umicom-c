/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_comparison_wire.c
 *
 * PURPOSE:
 *   Verify exact legacy bytes and explicit comparison format compatibility with
 * independent wire fixtures.
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
static int Is(const char *a, const char *b) { return strcmp(a, b) == 0; }
int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  const char *mode = argv[1];
  const char *cases[] = {"legacy-stable",   "comparison-roundtrip",
                         "legacy-mislabel", "comparison-mislabel",
                         "unknown-version", "reserved-flags",
                         "unknown-opcode",  "truncated",
                         "trailing",        "small-capacity"};
  size_t known = 0U;
  for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i)
    known += (size_t)Is(mode, cases[i]);
  if (known != 1U)
    return 2;
  /* Explicit record bytes are independent of the encoder's implementation. */
  static const uint8_t legacy[] = {'U', 'M', 'C', 'B', 1, 0, 2, 0, 1,
                                   42,  0,   0,   0,   0, 0, 0, 0, 7,
                                   0,   0,   0,   0,   0, 0, 0, 0};
  static const uint8_t comparison[] = {
      'U', 'M', 'C', 'B', 2, 0, 4, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0,
      2,   0,   0,   0,   8, 0, 0, 0, 0, 1, 0, 0, 0, 7, 0, 0, 0, 0, 3, 0, 0, 0};
  uint8_t bytes[UMC_BYTECODE_LIMIT];
  UmcInstruction p[4];
  size_t count = 99U;
  const uint8_t *fixture =
      Is(mode, "legacy-stable") || Is(mode, "legacy-mislabel") ? legacy
                                                               : comparison;
  size_t length = fixture == legacy ? sizeof(legacy) : sizeof(comparison);
  memcpy(bytes, fixture, length);
  if (Is(mode, "legacy-mislabel"))
    bytes[4] = 2U;
  if (Is(mode, "comparison-mislabel"))
    bytes[4] = 1U;
  if (Is(mode, "unknown-version"))
    bytes[4] = 3U;
  if (Is(mode, "reserved-flags"))
    bytes[5] = 1U;
  if (Is(mode, "unknown-opcode"))
    bytes[8U + 2U * 9U] = 14U;
  if (Is(mode, "trailing"))
    bytes[length++] = 0U;
  if (Is(mode, "truncated")) {
    for (size_t n = 0U; n < length; ++n) {
      count = 99U;
      CHECK(umc_decode(bytes, n, p, 4U, &count, NULL) == UMC_BAD_BYTECODE &&
            count == 0U);
    }
    return 0;
  }
  UmcStatus status = umc_decode(
      bytes, length, p, Is(mode, "small-capacity") ? 3U : 4U, &count, NULL);
  if (Is(mode, "small-capacity")) {
    CHECK(status == UMC_BUFFER_TOO_SMALL && count == 0U);
    return 0;
  }
  if (!Is(mode, "legacy-stable") && !Is(mode, "comparison-roundtrip")) {
    CHECK(status == UMC_BAD_BYTECODE && count == 0U);
    return 0;
  }
  CHECK(status == UMC_OK);
  UmcResult result;
  CHECK(umc_execute(p, count, count, &result, NULL) == UMC_OK);
  CHECK(result.value == (fixture == legacy ? 42 : 1));
  size_t encoded = 0U;
  CHECK(umc_encode(p, count, bytes, sizeof(bytes), &encoded, NULL) == UMC_OK);
  CHECK(encoded == length && memcmp(bytes, fixture, length) == 0);
  return 0;
}
