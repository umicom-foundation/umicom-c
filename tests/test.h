/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test.h
 *
 * PURPOSE:
 *   Keep test assertions active in both Debug and Release configurations.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#ifndef UMC_TEST_H
#define UMC_TEST_H
#include "umicom_c/umicom_c.h"
#include <stdio.h>
#include <string.h>
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)
#endif
