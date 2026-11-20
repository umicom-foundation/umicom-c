/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_int_input.c
 *
 * PURPOSE:
 *   Qualify the required-input artifact and the native binding boundary.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "umicom_c/int_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

static void expect_execution(const uint8_t *bytes, size_t length, int32_t input, int32_t expected)
{
    UmcInstruction output[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    size_t count = 0U;
    CHECK(umc_bind_int_input_bytecode(bytes, length, input, output, UMC_PROGRAM_LIMIT,
        &count, &diagnostic) == UMC_OK);
    CHECK(count > 2U && output[0].opcode == UMC_PUSH && output[0].operand == input);
    CHECK(output[1].opcode == UMC_STORE_LOCAL && output[1].operand == 0);
    UmcResult result = {0};
    CHECK(umc_execute(output, count, UMC_PROGRAM_LIMIT, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == expected && result.instructions_executed > 2U);
}

static void refuse_binding(const uint8_t *bytes, size_t length, size_t capacity, UmcStatus expected)
{
    UmcInstruction output[UMC_PROGRAM_LIMIT];
    unsigned char before[sizeof output];
    UmcDiagnostic diagnostic;
    memset(output, 0xa5, sizeof output);
    memcpy(before, output, sizeof before);
    size_t count = 42U;
    CHECK(umc_bind_int_input_bytecode(bytes, length, 12, output, capacity, &count, &diagnostic) == expected);
    CHECK(count == 0U && memcmp(before, output, sizeof before) == 0);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    const char *mode = argv[1];
    static const char source[] = "int umicom_run(int input) { return input; }";
    uint8_t bytes[UMC_BYTECODE_LIMIT];
    size_t length = 0U;
    UmcDiagnostic diagnostic;
    CHECK(umc_compile_int_input(source, sizeof source - 1U, bytes, sizeof bytes, &length, &diagnostic) == UMC_OK);
    CHECK(length == UMC_BYTECODE_HEADER_SIZE + 4U * UMC_BYTECODE_RECORD_SIZE && bytes[4] == 3U &&
        bytes[5] == UMC_INT_INPUT_BYTECODE_FLAG);

    if (strcmp(mode, "values") == 0) {
        uint8_t original[UMC_BYTECODE_LIMIT];
        memcpy(original, bytes, length);
        const int32_t values[] = {0, 42, -17, INT32_MIN, INT32_MAX};
        for (size_t i = 0U; i < sizeof values / sizeof values[0]; ++i)
            expect_execution(bytes, length, values[i], values[i]);
        CHECK(memcmp(original, bytes, length) == 0);
    } else if (strcmp(mode, "statements") == 0) {
        static const char calculation[] = "int umicom_run(int x) { int total=x*2; if(x>0) { total+=2; } return total; }";
        CHECK(umc_compile_int_input(calculation, sizeof calculation - 1U, bytes, sizeof bytes,
            &length, &diagnostic) == UMC_OK);
        expect_execution(bytes, length, 20, 42);
        expect_execution(bytes, length, -5, -10);
    } else if (strcmp(mode, "legacy") == 0) {
        UmcInstruction output[UMC_PROGRAM_LIMIT];
        size_t count = 0U;
        CHECK(umc_decode(bytes, length, output, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_BAD_BYTECODE);
        CHECK(count == 0U);
        CHECK(umc_compile(source, sizeof source - 1U, output, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        static const char old[] = "int main(void) { return 42; }";
        CHECK(umc_compile(old, sizeof old - 1U, output, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
        CHECK(umc_encode(output, count, bytes, sizeof bytes, &length, &diagnostic) == UMC_OK && bytes[5] == 0U);
        refuse_binding(bytes, length, UMC_PROGRAM_LIMIT, UMC_BAD_BYTECODE);
    } else if (strcmp(mode, "prefix") == 0) {
        /* Replace the zero placeholder with an ordinary constant. The rest
         * remains a valid locals program, so the input boundary must refuse. */
        bytes[UMC_BYTECODE_HEADER_SIZE + 1U] = 1U;
        refuse_binding(bytes, length, UMC_PROGRAM_LIMIT, UMC_BAD_BYTECODE);
    } else if (strcmp(mode, "flags") == 0) {
        bytes[5] = 3U;
        refuse_binding(bytes, length, UMC_PROGRAM_LIMIT, UMC_BAD_BYTECODE);
        bytes[5] = UMC_INT_INPUT_BYTECODE_FLAG;
        refuse_binding(bytes, length - 1U, UMC_PROGRAM_LIMIT, UMC_BAD_BYTECODE);
    } else if (strcmp(mode, "capacity") == 0) {
        refuse_binding(bytes, length, 3U, UMC_BUFFER_TOO_SMALL);
        uint8_t marker[UMC_BYTECODE_LIMIT];
        memset(bytes, 0x77, sizeof bytes);
        memcpy(marker, bytes, sizeof marker);
        CHECK(umc_compile_int_input(source, sizeof source - 1U, bytes, 8U, &length,
            &diagnostic) == UMC_BUFFER_TOO_SMALL);
        CHECK(length == 0U && memcmp(bytes, marker, sizeof bytes) == 0);
    } else if (strcmp(mode, "source") == 0) {
        const char *invalid[] = {"int main(void) { return 1; }", "int umicom_run(void) { return 1; }",
            "int umicom_run(int x, int y) { return x; }", "int umicom_run(int x) { int x=40; return x; }",
            "int umicom_run(int) { return 1; }"};
        uint8_t before[UMC_BYTECODE_LIMIT];
        memset(bytes, 0xab, sizeof bytes);
        memcpy(before, bytes, sizeof before);
        for (size_t i = 0U; i < sizeof invalid / sizeof invalid[0]; ++i) {
            CHECK(umc_compile_int_input(invalid[i], strlen(invalid[i]), bytes, sizeof bytes,
                &length, &diagnostic) == UMC_SYNTAX_ERROR);
            CHECK(length == 0U && diagnostic.line == 1U && memcmp(bytes, before, sizeof before) == 0);
        }
    } else if (strcmp(mode, "arithmetic") == 0) {
        static const char addition[] = "int umicom_run(int x) { return x+1; }";
        CHECK(umc_compile_int_input(addition, sizeof addition - 1U, bytes, sizeof bytes,
            &length, &diagnostic) == UMC_OK);
        UmcInstruction output[UMC_PROGRAM_LIMIT];
        size_t count = 0U;
        UmcResult result = {123, 4U};
        CHECK(umc_bind_int_input_bytecode(bytes, length, INT32_MAX, output, UMC_PROGRAM_LIMIT,
            &count, &diagnostic) == UMC_OK);
        CHECK(umc_execute(output, count, UMC_PROGRAM_LIMIT, &result, &diagnostic) == UMC_ARITHMETIC_ERROR);
        CHECK(result.value == 123 && result.instructions_executed == 4U);
        CHECK(umc_execute(output, count, 1U, &result, &diagnostic) == UMC_FUEL_EXHAUSTED);
        CHECK(result.value == 123 && result.instructions_executed == 4U);
    } else {
        CHECK(0);
    }
    return 0;
}
