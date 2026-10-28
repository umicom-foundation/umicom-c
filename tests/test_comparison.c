/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_comparison.c
 *
 * PURPOSE:
 *   Compare validated operations separately from diagnostic source locations.
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
#include <string.h>
int main(void)
{
    const char *plain = "int main(void) { return 7 * 6; }";
    const char *comment = "/* Learn multiplication. */\nint main(void) { return 7 /* units */ * 6; }";
    UmcInstruction left[UMC_PROGRAM_LIMIT], right[UMC_PROGRAM_LIMIT];
    size_t a = 0U, b = 0U;
    UmcDiagnostic diagnostic;
    bool equal = false;
    CHECK(umc_compile(plain, strlen(plain), left, UMC_PROGRAM_LIMIT, &a, &diagnostic) == UMC_OK);
    CHECK(umc_compile(comment, strlen(comment), right, UMC_PROGRAM_LIMIT, &b, &diagnostic) == UMC_OK);
    CHECK(left[0].source_offset != right[0].source_offset);
    CHECK(umc_program_same_operations(left, a, right, b, &equal, &diagnostic) == UMC_OK && equal);
    right[0].operand = 8;
    CHECK(umc_program_same_operations(left, a, right, b, &equal, &diagnostic) == UMC_OK && !equal);
    right[0].opcode = (UmcOpcode)255;
    equal = true;
    CHECK(umc_program_same_operations(left, a, right, b, &equal, &diagnostic) == UMC_BAD_BYTECODE && !equal);
    CHECK(umc_program_same_operations(left, a, left, a, NULL, &diagnostic) == UMC_INVALID_ARGUMENT);
    return 0;
}
