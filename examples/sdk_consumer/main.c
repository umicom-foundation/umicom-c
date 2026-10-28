/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/sdk_consumer/main.c
 *
 * PURPOSE:
 *   Use the installed compiler SDK without building the source project.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include <inttypes.h>
#include "umicom_c/umicom_c.h"

int main(void)
{
    static const char source[] = "int main(void) { return 6 * 7; }";
    UmcInstruction program[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    UmcResult result;
    size_t count = 0U;
    if (umc_compile(source, sizeof(source) - 1U, program, UMC_PROGRAM_LIMIT,
        &count, &diagnostic) != UMC_OK ||
        umc_execute(program, count, count, &result, &diagnostic) != UMC_OK) {
        fprintf(stderr, "%s\n", diagnostic.message);
        return 1;
    }
    printf("Result: %" PRId32 "\n", result.value);
    return result.value == 42 ? 0 : 1;
}
