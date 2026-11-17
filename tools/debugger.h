/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tools/debugger.h
 *
 * PURPOSE:
 *   Keep hosted console interaction separate from the freestanding VM.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMC_DEBUGGER_H
#define UMC_DEBUGGER_H
#include "umicom_c/umicom_c.h"
#include <stdio.h>
int umc_debug_console(const char *source, size_t length,
    const UmcInstruction *program, size_t count, size_t fuel,
    FILE *input, FILE *output, FILE *errors);
#endif
