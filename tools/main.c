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
#include "umicom_c/trace.h"
#include "umicom_c/profile.h"
#include "umicom_c/comparison.h"
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
          "  umicom-c compare-operations left.uc right.uc\n"
          "  umicom-c inspect source.uc\n"
          "  umicom-c inspect-bytecode program.umcb\n"
          "  umicom-c run source.uc [--max-steps N]\n"
          "  umicom-c compile source.uc new-output.umcb\n"
          "  umicom-c exec program.umcb [--max-steps N]\n"
          "  umicom-c disassemble program.umcb\n"
          "  umicom-c trace source.uc [--max-steps N]\n"
          "  umicom-c trace-bytecode program.umcb [--max-steps N]\n"
          "  umicom-c --version\n"
          "  umicom-c profile\n"
          /* Preserve the earlier profile wording while exposing the additive comparison grammar. */
#if 0
          "Profile: int main(void) { return <int32 arithmetic>; }\n", stderr);
#endif
          /* The return-expression profile is retained as a valid subset. */
#if 0
          "Profile: int main(void) { return <int32 arithmetic or comparison>; }\n", stderr);
#endif
          "Profile: int main(void) { [initialized int locals and assignments] return <int32 expression>; }\n", stderr);
    return 2;
}

/* Keep execution policy in the caller: the runtime already accounts for each
 * instruction. Parse a bounded decimal value without locale, sign or overflow
 * so a typo cannot silently expand the requested execution allowance. */
static int step_limit(const char *text, size_t *limit)
{
    size_t value = 0U;
    if (text == NULL || text[0] == '\0') return 0;
    for (size_t i = 0U; text[i] != '\0'; ++i) {
        if (text[i] < '0' || text[i] > '9') return 0;
        const size_t digit = (size_t)(text[i] - '0');
        if (value > (UMC_PROGRAM_LIMIT - digit) / 10U) return 0;
        value = value * 10U + digit;
        if (i >= 9U) return 0;
    }
    if (value == 0U) return 0;
    *limit = value;
    return 1;
}

/* This command does not open a source file or execute a program. Separate
 * component keys keep source features distinct from runtime and wire support. */
/* Append local capacity keys while preserving the existing machine-readable profile schema and component separation. The predecessor remains disabled for review. */
#if 0
static int print_profile(void)
{
    printf("profile.schema=1\n"
        "compiler.api=%" PRIu32 "\ncompiler.features=%" PRIu32 "\n"
        "compiler.source-bytes=%" PRIu32 "\ncompiler.instructions=%" PRIu32 "\n"
        "compiler.parse-depth=%" PRIu32 "\n"
        "runtime.api=%" PRIu32 "\nruntime.features=%" PRIu32 "\n"
        "runtime.instructions=%" PRIu32 "\nruntime.stack-values=%" PRIu32 "\n"
        "runtime.bytecode-bytes=%" PRIu32 "\nruntime.wire-versions=%" PRIu32 "\n",
        umc_compiler_profile(UMC_PROFILE_API), umc_compiler_profile(UMC_PROFILE_FEATURES),
        umc_compiler_profile(UMC_PROFILE_SOURCE_BYTES), umc_compiler_profile(UMC_PROFILE_PROGRAM_INSTRUCTIONS),
        umc_compiler_profile(UMC_PROFILE_PARSE_DEPTH),
        umc_runtime_profile(UMC_PROFILE_API), umc_runtime_profile(UMC_PROFILE_FEATURES),
        umc_runtime_profile(UMC_PROFILE_PROGRAM_INSTRUCTIONS), umc_runtime_profile(UMC_PROFILE_STACK_VALUES),
        umc_runtime_profile(UMC_PROFILE_BYTECODE_BYTES), umc_runtime_profile(UMC_PROFILE_WIRE_VERSIONS));
    return ferror(stdout) ? 1 : 0;
}

#endif

static int print_profile(void)
{
    printf("profile.schema=1\n"
        "compiler.api=%" PRIu32 "\ncompiler.features=%" PRIu32 "\n"
        "compiler.source-bytes=%" PRIu32 "\ncompiler.instructions=%" PRIu32 "\n"
        "compiler.parse-depth=%" PRIu32 "\n"
        "runtime.api=%" PRIu32 "\nruntime.features=%" PRIu32 "\n"
        "runtime.instructions=%" PRIu32 "\nruntime.stack-values=%" PRIu32 "\n"
        "runtime.bytecode-bytes=%" PRIu32 "\nruntime.wire-versions=%" PRIu32 "\n",
        umc_compiler_profile(UMC_PROFILE_API), umc_compiler_profile(UMC_PROFILE_FEATURES),
        umc_compiler_profile(UMC_PROFILE_SOURCE_BYTES), umc_compiler_profile(UMC_PROFILE_PROGRAM_INSTRUCTIONS),
        umc_compiler_profile(UMC_PROFILE_PARSE_DEPTH),
        umc_runtime_profile(UMC_PROFILE_API), umc_runtime_profile(UMC_PROFILE_FEATURES),
        umc_runtime_profile(UMC_PROFILE_PROGRAM_INSTRUCTIONS), umc_runtime_profile(UMC_PROFILE_STACK_VALUES),
        umc_runtime_profile(UMC_PROFILE_BYTECODE_BYTES), umc_runtime_profile(UMC_PROFILE_WIRE_VERSIONS));
    printf("compiler.local-values=%" PRIu32 "\ncompiler.identifier-bytes=%" PRIu32 "\nruntime.local-values=%" PRIu32 "\n",
        umc_compiler_profile(UMC_PROFILE_LOCAL_VALUES),
        umc_compiler_profile(UMC_PROFILE_IDENTIFIER_BYTES),
        umc_runtime_profile(UMC_PROFILE_LOCAL_VALUES));
    return ferror(stdout) ? 1 : 0;
}


/* File ownership stays in the hosted command. The existing core comparison
 * service decides validated opcode/operand identity; the CLI neither executes
 * either program nor invents a second semantic comparison implementation. */
static int compare_operations(const char *left_path, const char *right_path)
{
    uint8_t source[UMC_SOURCE_LIMIT];
    UmcInstruction left[UMC_PROGRAM_LIMIT], right[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    size_t counts[2] = {0U, 0U};
    const char *paths[2] = {left_path, right_path};
    UmcInstruction *programs[2] = {left, right};
    for (size_t side = 0U; side < 2U; ++side) {
        size_t bytes = 0U;
        if (!read_file(paths[side], source, sizeof source, &bytes)) return 2;
        if (umc_compile((const char *)source, bytes, programs[side],
            UMC_PROGRAM_LIMIT, &counts[side], &diagnostic) != UMC_OK) {
            fprintf(stderr, "%s:%zu:%zu: %s\n", paths[side], diagnostic.line,
                diagnostic.column, diagnostic.message);
            return 2;
        }
    }
    bool equal = false;
    if (umc_program_same_operations(left, counts[0], right, counts[1],
        &equal, &diagnostic) != UMC_OK) {
        fprintf(stderr, "Cannot compare operations: %s\n", diagnostic.message);
        return 2;
    }
    printf("operations=%s\nleft.instructions=%zu\nright.instructions=%zu\n"
        "source-locations=not-compared\nexecution=not-performed\n",
        equal ? "same" : "different", counts[0], counts[1]);
    /* Observe buffered-report failures before returning a semantic result.
     * The former check detected only errors already raised by printf; retain
     * it for review while flushing this command's short report explicitly. */
#if 0
    if (ferror(stdout)) return 2;
#endif
    if (fflush(stdout) != 0 || ferror(stdout)) return 2;
    /* Deliberately distinguish a valid difference from invalid input. This
     * command-specific result must not be mistaken for general C equivalence,
     * matching diagnostic locations, binary identity or build-cache approval. */
    return equal ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "compare-operations") == 0)
        return argc == 4 ? compare_operations(argv[2], argv[3]) : usage();
    if (argc >= 2 && strcmp(argv[1], "profile") == 0)
        return argc == 2 ? print_profile() : usage();
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        /* Older artifacts keep format 1; comparisons explicitly require format 2.
         * Retain the previous capability string for compatibility review. */
#if 0
        puts("Umicom C 0.1.0; arithmetic profile; bytecode format 1");
#endif
        puts("Umicom C 0.1.0; arithmetic and comparison profiles; bytecode formats 1 and 2");
        return 0;
    }
    if (argc < 3) return usage();
    const int check = strcmp(argv[1], "check") == 0;
    const int run = strcmp(argv[1], "run") == 0;
    const int compile = strcmp(argv[1], "compile") == 0;
    const int execute = strcmp(argv[1], "exec") == 0;
    const int disassemble = strcmp(argv[1], "disassemble") == 0;
    const int inspect_source = strcmp(argv[1], "inspect") == 0;
    const int inspect_bytes = strcmp(argv[1], "inspect-bytecode") == 0;
    const int trace_source = strcmp(argv[1], "trace") == 0;
    const int trace_bytes = strcmp(argv[1], "trace-bytecode") == 0;
    const int tracing = trace_source || trace_bytes;
    /* The original fixed-arity rule is retained for review. The replacement
     * keeps all earlier commands and admits an execution-only budget option. */
#if 0
    if ((!check && !run && !compile && !execute && !disassemble) ||
        argc != (compile ? 4 : 3)) return usage();
#endif
    size_t fuel = UMC_PROGRAM_LIMIT;
    /* Keep the previous command gate for review; inspection is an additive
     * read-only command and shares the compiler/runtime validation path. */
#if 0
    if (!check && !run && !compile && !execute && !disassemble) return usage();
#endif
    /* Tracing executes the existing arithmetic profile with caller-owned output.
     * Preserve the earlier gate while extending its exact budget/arity checks. */
#if 0
    if (!check && !run && !compile && !execute && !disassemble &&
        !inspect_source && !inspect_bytes) return usage();
    if ((run || execute) && argc == 5) {
#endif
    if (!check && !run && !compile && !execute && !disassemble &&
        !inspect_source && !inspect_bytes && !tracing) return usage();
    if ((run || execute || tracing) && argc == 5) {
        if (strcmp(argv[3], "--max-steps") != 0 || !step_limit(argv[4], &fuel)) {
            fprintf(stderr, "--max-steps requires a decimal integer from 1 to %u.\n",
                (unsigned)UMC_PROGRAM_LIMIT);
            return 2;
        }
    } else if (argc != (compile ? 4 : 3)) return usage();

    /* The command owns these arrays for its lifetime. Core libraries never
     * allocate, retain them or perform file operations on the caller's behalf. */
    uint8_t input[UMC_SOURCE_LIMIT];
    UmcInstruction instructions[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    size_t length = 0U;
    size_t count = 0U;
    /* Binary inspection uses the decoder, including all wire-format checks.
     * Retain the earlier selector to make the extension explicit. */
#if 0
    const int binary = execute || disassemble;
#endif
    /* Decode trace-bytecode through the same complete wire validator. The
     * former selector is retained to document the additive command boundary. */
#if 0
    const int binary = execute || disassemble || inspect_bytes;
#endif
    const int binary = execute || disassemble || inspect_bytes || trace_bytes;
    if (!read_file(argv[2], input, binary ? UMC_BYTECODE_LIMIT : UMC_SOURCE_LIMIT, &length)) {
        return 2;
    }
    UmcStatus status = binary ?
        umc_decode(input, length, instructions, UMC_PROGRAM_LIMIT, &count, &diagnostic) :
        umc_compile((const char *)input, length, instructions, UMC_PROGRAM_LIMIT, &count, &diagnostic);
    if (status == UMC_OK && (inspect_source || inspect_bytes)) {
        UmcProgramInfo info;
        status = umc_inspect(instructions, count, &info, &diagnostic);
        if (status == UMC_OK) {
            printf("Instructions: %zu\nPeak stack values: %zu\nConstants: %zu\n"
                   "Arithmetic operations: %zu\nEncoded bytes: %zu\n",
                info.instruction_count, info.peak_stack, info.constant_count,
                info.arithmetic_count, info.bytecode_size);
            puts("Structural inspection only; arithmetic has not been executed.");
        }
    } else if (status == UMC_OK && compile) {
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
        /* Preserve the former default call; explicit fuel now reaches the same
         * runtime, including its unchanged-result guarantee on exhaustion. */
#if 0
        status = umc_execute(instructions, count, UMC_PROGRAM_LIMIT, &result, &diagnostic);
#endif
        /* Keep the old dispatch for review. Only trace commands request storage;
         * both branches now reach the same runtime executor and failure rules. */
#if 0
        status = umc_execute(instructions, count, fuel, &result, &diagnostic);
#endif
        if (tracing) {
            UmcTraceStep steps[UMC_PROGRAM_LIMIT];
            size_t used = 0U;
            status = umc_execute_trace(instructions, count, fuel, &result,
                steps, UMC_PROGRAM_LIMIT, &used, &diagnostic);
            for (size_t i = 0U; i < used; ++i) {
                printf("step=%zu source-byte=%zu opcode=%s stack-depth=%zu top=%" PRId32 " status=%s\n",
                    steps[i].instruction_index, steps[i].source_offset,
                    umc_opcode_text(steps[i].opcode), steps[i].stack_depth,
                    steps[i].top, umc_status_text(steps[i].status));
            }
        } else {
            status = umc_execute(instructions, count, fuel, &result, &diagnostic);
        }
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
