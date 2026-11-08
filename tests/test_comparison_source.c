/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_comparison_source.c
 *
 * PURPOSE:
 *   Check comparison truth values, C-style precedence, exact operator
 * diagnostics and unsupported syntax.
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
typedef struct Case {
  const char *name, *expression;
  int32_t value;
  UmcStatus status;
} Case;
static const Case cases[] = {
    {"less", "1 < 2", 1, UMC_OK},
    {"less-false", "2 < 1", 0, UMC_OK},
    {"less-equal", "2 <= 2", 1, UMC_OK},
    {"greater", "3 > 2", 1, UMC_OK},
    {"greater-equal", "3 >= 4", 0, UMC_OK},
    {"equal", "7 == 7", 1, UMC_OK},
    {"not-equal", "7 != 8", 1, UMC_OK},
    {"negative", "-3 < -2", 1, UMC_OK},
    {"minimum", "(-2147483647 - 1) < 2147483647", 1, UMC_OK},
    {"maximum", "2147483647 > (-2147483647 - 1)", 1, UMC_OK},
    {"additive-first", "2 + 3 < 2 * 3", 1, UMC_OK},
    {"relational-first", "2 == 3 < 4", 0, UMC_OK},
    {"equality-left", "1 == 2 == 0", 1, UMC_OK},
    {"relational-left", "1 < 2 < 1", 0, UMC_OK},
    {"parentheses", "(1 < 2) + (3 == 3)", 2, UMC_OK},
    {"unary-result", "-(1 < 2)", -1, UMC_OK},
    {"nested", "((2 + 3) >= (7 - 2)) != 0", 1, UMC_OK},
    {"comment-boundary", "2 /* left */ <= /* right */ 2", 1, UMC_OK},
    {"both-operands", "0 == (1 / 0)", 0, UMC_ARITHMETIC_ERROR},
    {"bare-assignment", "1 = 2", 0, UMC_SYNTAX_ERROR},
    {"bare-not", "!1", 0, UMC_SYNTAX_ERROR},
    {"spaced-operator", "1 < = 2", 0, UMC_SYNTAX_ERROR},
    {"missing-right", "1 <=", 0, UMC_SYNTAX_ERROR},
    {"logical-and", "1 && 2", 0, UMC_SYNTAX_ERROR}};
int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  const Case *chosen = NULL;
  for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i)
    if (strcmp(argv[1], cases[i].name) == 0)
      chosen = &cases[i];
  if (chosen == NULL)
    return 2;
  char source[256];
  int bytes =
      snprintf(source, sizeof(source), "int main(void) {\r\n return %s;\r\n}",
               chosen->expression);
  CHECK(bytes > 0 && (size_t)bytes < sizeof(source));
  UmcInstruction program[UMC_PROGRAM_LIMIT];
  size_t count = 99U;
  UmcDiagnostic diagnostic;
  UmcStatus status = umc_compile(source, (size_t)bytes, program,
                                 UMC_PROGRAM_LIMIT, &count, &diagnostic);
  if (chosen->status == UMC_SYNTAX_ERROR) {
    CHECK(status == chosen->status && count == 0U);
    CHECK(diagnostic.line == 2U && diagnostic.column >= 9U);
    return 0;
  }
  CHECK(status == UMC_OK && count > 0U);
  UmcResult result = {123, 321U};
  UmcTraceStep steps[UMC_PROGRAM_LIMIT];
  size_t used = 0U;
  status = umc_execute_trace(program, count, count, &result, steps,
                             UMC_PROGRAM_LIMIT, &used, &diagnostic);
  CHECK(status == chosen->status);
  if (status == UMC_OK) {
    CHECK(result.value == chosen->value &&
          result.instructions_executed == count && used == count);
  } else {
    CHECK(result.value == 123 && result.instructions_executed == 321U);
    CHECK(used > 0U && steps[used - 1U].opcode == UMC_DIVIDE);
    return 0;
  }
  /* The compiler records each comparison at its actual operator byte. */
  for (size_t i = 0U; i < count; ++i) {
    UmcOpcode op = program[i].opcode;
    if (op < UMC_LESS || op > UMC_NOT_EQUAL)
      continue;
    size_t offset = program[i].source_offset;
    CHECK(offset < (size_t)bytes);
    CHECK(source[offset] == (op == UMC_LESS || op == UMC_LESS_EQUAL ? '<'
                             : op == UMC_GREATER || op == UMC_GREATER_EQUAL
                                 ? '>'
                             : op == UMC_EQUAL ? '='
                                               : '!'));
    CHECK(steps[i].stack_depth > 0U &&
          (steps[i].top == 0 || steps[i].top == 1));
  }
  return 0;
}
