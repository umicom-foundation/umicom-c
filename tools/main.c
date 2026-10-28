/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tools/main.c
 *
 * PURPOSE:
 *   Expose source checking, bytecode compilation and bounded preview through explicit commands.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "umicom_c/umicom_c.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int read_file(const char *name, uint8_t *bytes, size_t capacity, size_t *length)
{
    FILE *stream = fopen(name, "rb");
    if (stream == NULL) {
        fprintf(stderr, "Cannot open input: %s\n", name);
        return 0;
    }
    const size_t used = fread(bytes, 1U, capacity, stream);
    const int extra = fgetc(stream);
    const int failed = ferror(stream);
    const int closed = fclose(stream);
    if (failed || closed != 0 || extra != EOF) {
        fprintf(stderr, "Input exceeds its limit or could not be read: %s\n", name);
        return 0;
    }
    *length = used;
    return 1;
}

static int save_new_file(const char *name, const uint8_t *bytes, size_t length)
{
    /* Exclusive creation preserves a source or previously compiled artifact,
     * including when the caller accidentally supplies the input path as output. */
    FILE *stream = fopen(name, "wbx");
    if (stream == NULL) {
        fprintf(stderr, "Cannot create output (existing files are preserved): %s\n", name);
        return 0;
    }
    const int wrote = fwrite(bytes, 1U, length, stream) == length;
    const int closed = fclose(stream) == 0;
    if (!wrote || !closed) {
        if (remove(name) != 0) {
            fprintf(stderr, "Incomplete newly-created output remains: %s\n", name);
        }
        fprintf(stderr, "Could not finish writing output: %s\n", name);
        return 0;
    }
    return 1;
}

static int usage(void)
{
    fputs("Usage:\n"
          "  umicom-c check source.uc\n"
          "  umicom-c run source.uc\n"
          "  umicom-c compile source.uc new-output.umcb\n"
          "  umicom-c exec program.umcb\n"
          "  umicom-c disassemble program.umcb\n"
          "  umicom-c --version\n"
          "Profile: int main(void) { return <int32 arithmetic>; }\n", stderr);
    return 2;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts("Umicom C 0.1.0; arithmetic profile; bytecode format 1");
        return 0;
    }
    if (argc < 3) return usage();
    const int check = strcmp(argv[1], "check") == 0;
    const int run = strcmp(argv[1], "run") == 0;
    const int compile = strcmp(argv[1], "compile") == 0;
    const int execute = strcmp(argv[1], "exec") == 0;
    const int disassemble = strcmp(argv[1], "disassemble") == 0;
    if ((!check && !run && !compile && !execute && !disassemble) ||
        argc != (compile ? 4 : 3)) return usage();

    /* The command owns these arrays for its lifetime. Core libraries never
     * allocate, retain them or perform file operations on the caller's behalf. */
    uint8_t input[UMC_SOURCE_LIMIT];
    UmcInstruction instructions[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    size_t length = 0U;
    size_t count = 0U;
    const int binary = execute || disassemble;
    if (!read_file(argv[2], input, binary ? UMC_BYTECODE_LIMIT : UMC_SOURCE_LIMIT, &length)) {
        return 2;
    }
    UmcStatus status = binary ?
        umc_decode(input, length, instructions, UMC_PROGRAM_LIMIT, &count, &diagnostic) :
        umc_compile((const char *)input, length, instructions, UMC_PROGRAM_LIMIT, &count, &diagnostic);
    if (status == UMC_OK && compile) {
        uint8_t encoded[UMC_BYTECODE_LIMIT];
        size_t bytes = 0U;
        status = umc_encode(instructions, count, encoded, sizeof(encoded), &bytes, &diagnostic);
        if (status == UMC_OK) {
            if (!save_new_file(argv[3], encoded, bytes)) return 2;
            printf("Compiled %zu instructions to %s\n", count, argv[3]);
        }
    } else if (status == UMC_OK && check) {
        printf("Syntax and bytecode structure accepted: %zu instructions\n", count);
    } else if (status == UMC_OK && disassemble) {
        for (size_t i = 0U; i < count; ++i) {
            printf("%zu  %-8s %" PRId32 "  source-byte=%zu\n", i,
                umc_opcode_text(instructions[i].opcode), instructions[i].operand,
                instructions[i].source_offset);
        }
    } else if (status == UMC_OK) {
        UmcResult result;
        status = umc_execute(instructions, count, UMC_PROGRAM_LIMIT, &result, &diagnostic);
        if (status == UMC_OK) {
            printf("Result: %" PRId32 "\nInstructions: %zu\n", result.value, result.instructions_executed);
        }
    }
    if (status != UMC_OK) {
        if (!binary) umc_locate_diagnostic((const char *)input, length, &diagnostic);
        if (diagnostic.line != 0U) {
            fprintf(stderr, "%s:%zu:%zu: %s\n", argv[2], diagnostic.line,
                diagnostic.column, diagnostic.message);
        } else {
            fprintf(stderr, "%s: source-byte=%zu: %s\n", argv[2],
                diagnostic.source_offset, diagnostic.message);
        }
        return 1;
    }
    return 0;
}
