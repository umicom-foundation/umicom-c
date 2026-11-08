/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_comparison_runtime.c
 *
 * PURPOSE:
 *   Exercise canonical comparison execution at signed limits, fuel boundaries
 * and malformed bytecode admission.
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
static int Is(const char *a, const char *b) { return strcmp(a, b) == 0; }
int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  const char *mode = argv[1];
  const char *cases[] = {"truth-matrix",  "extreme-values", "fuel-boundaries",
                         "trace",         "underflow",      "reserved-operand",
                         "inspection",    "short-trace",    "last-opcode",
                         "unknown-opcode"};
  size_t known = 0U;
  for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i)
    known += (size_t)Is(mode, cases[i]);
  if (known != 1U)
    return 2;
  const UmcOpcode operations[] = {UMC_LESS,    UMC_LESS_EQUAL,
                                  UMC_GREATER, UMC_GREATER_EQUAL,
                                  UMC_EQUAL,   UMC_NOT_EQUAL};
  const int32_t left[] = {-5, 0, 9, INT32_MIN, INT32_MAX};
  const int32_t right[] = {2, 0, -3, INT32_MAX, INT32_MIN};
  const int32_t expected[5][6] = {{1, 1, 0, 0, 0, 1},
                                  {0, 1, 0, 1, 1, 0},
                                  {0, 0, 1, 1, 0, 1},
                                  {1, 1, 0, 0, 0, 1},
                                  {0, 0, 1, 1, 0, 1}};
  if (Is(mode, "truth-matrix") || Is(mode, "extreme-values")) {
    for (size_t row = Is(mode, "extreme-values") ? 3U : 0U; row < 5U; ++row)
      for (size_t col = 0U; col < 6U; ++col) {
        UmcInstruction p[] = {{UMC_PUSH, left[row], 0U},
                              {UMC_PUSH, right[row], 2U},
                              {operations[col], 0, 1U},
                              {UMC_RETURN, 0, 3U}};
        UmcResult result;
        CHECK(umc_execute(p, 4U, 4U, &result, NULL) == UMC_OK);
        CHECK(result.value == expected[row][col] &&
              result.instructions_executed == 4U);
      }
    return 0;
  }
  UmcInstruction p[] = {{UMC_PUSH, INT32_MIN, 0U},
                        {UMC_PUSH, INT32_MAX, 2U},
                        {UMC_LESS, 0, 1U},
                        {UMC_RETURN, 0, 3U}};
  UmcResult result = {19, 17U};
  UmcDiagnostic diagnostic;
  UmcTraceStep steps[4];
  size_t used = 99U;
  if (Is(mode, "fuel-boundaries")) {
    for (size_t fuel = 0U; fuel < 4U; ++fuel) {
      CHECK(umc_execute_trace(p, 4U, fuel, &result, steps, 4U, &used,
                              &diagnostic) == UMC_FUEL_EXHAUSTED);
      CHECK(used == fuel + 1U && steps[fuel].status == UMC_FUEL_EXHAUSTED &&
            result.value == 19 && result.instructions_executed == 17U);
    }
  } else if (Is(mode, "trace")) {
    CHECK(umc_execute_trace(p, 4U, 4U, &result, steps, 4U, &used,
                            &diagnostic) == UMC_OK);
    CHECK(used == 4U && steps[2].opcode == UMC_LESS &&
          steps[2].stack_depth == 1U && steps[2].top == 1 && steps[3].top == 1);
  } else if (Is(mode, "inspection")) {
    UmcProgramInfo info;
    CHECK(umc_inspect(p, 4U, &info, &diagnostic) == UMC_OK);
    CHECK(info.instruction_count == 4U && info.peak_stack == 2U &&
          info.constant_count == 2U && info.arithmetic_count == 0U &&
          info.bytecode_size == 44U);
  } else if (Is(mode, "short-trace")) {
    steps[0].top = 123;
    CHECK(umc_execute_trace(p, 4U, 4U, &result, steps, 3U, &used,
                            &diagnostic) == UMC_BUFFER_TOO_SMALL);
    CHECK(used == 0U && steps[0].top == 123 && result.value == 19);
  } else if (Is(mode, "last-opcode")) {
    p[2].opcode = UMC_NOT_EQUAL;
    CHECK(umc_execute(p, 4U, 4U, &result, &diagnostic) == UMC_OK &&
          result.value == 1);
  } else {
    if (Is(mode, "underflow"))
      p[0].opcode = UMC_LESS;
    if (Is(mode, "underflow"))
      p[0].operand = 0;
    if (Is(mode, "reserved-operand"))
      p[2].operand = 1;
    if (Is(mode, "unknown-opcode"))
      p[2].opcode = (UmcOpcode)14;
    CHECK(umc_execute_trace(p, 4U, 4U, &result, steps, 4U, &used,
                            &diagnostic) == UMC_BAD_BYTECODE);
    CHECK(used == 0U && result.value == 19 &&
          result.instructions_executed == 17U);
  }
  return 0;
}
