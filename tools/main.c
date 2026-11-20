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
#include "debugger.h"
#include "umicom_c/source_regions.h"
#include "umicom_c/trace.h"
#include "umicom_c/profile.h"
#include "umicom_c/execution_bounds.h"
#include "umicom_c/diagnostic_excerpt.h"
#include "umicom_c/comparison.h"
#include "umicom_c/source_check.h"
#include "umicom_c/translate.h"
#include "umicom_c/int_input.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* The hosted CLI owns report delivery. A successful short write may still be
 * buffered, so check the stream after requesting delivery before returning. */
static int report_write_failed(FILE *stream)
{
    return fflush(stream) != 0 || ferror(stream);
}

/* Source formatting belongs to the compiler service; the hosted client only
 * adds a gutter and writes its bounded printable-ASCII result. */
static void print_source_excerpt(const uint8_t *source, size_t length, size_t offset)
{
    UmcDiagnosticExcerpt excerpt;
    if (umc_diagnostic_excerpt((const char *)source, length, offset, &excerpt) != UMC_OK)
        return;
    fprintf(stderr, "  | %s\n  | %*s^\n", excerpt.text, (int)excerpt.caret_column, "");
}

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
          "  umicom-c check source.uc --warnings\n"
          "  umicom-c regions mixed.umc\n"
          "  umicom-c debug source.uc [--max-steps N]\n"
          "  umicom-c debug-bytecode program.umcb [--max-steps N]\n"
          "  umicom-c compare-operations left.uc right.uc\n"
          "  umicom-c inspect source.uc\n"
          "  umicom-c inspect-bytecode program.umcb\n"
          "  umicom-c budget source.uc\n"
          "  umicom-c budget-bytecode program.umcb\n"
          "  umicom-c run source.uc [--max-steps N]\n"
          "  umicom-c compile source.uc new-output.umcb\n"
          "  umicom-c compile-input source.uc new-output.umcb\n"
          "  umicom-c translate c-to-java|java-to-c input new-output\n"
          "  umicom-c exec program.umcb [--max-steps N]\n"
          "  umicom-c exec-input program.umcb SIGNED_INTEGER [--max-steps N]\n"
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

static int parse_int_input(const char *text, int32_t *out)
{
    const int negative = text[0] == '-';
    if (negative || text[0] == '+') ++text;
    if (text[0] == '\0') return 0;
    const uint32_t limit = (uint32_t)INT32_MAX + (negative ? 1U : 0U);
    uint32_t magnitude = 0U;
    for (; *text != '\0'; ++text) {
        if (*text < '0' || *text > '9') return 0;
        const uint32_t digit = (uint32_t)(*text - '0');
        if (magnitude > (limit - digit) / 10U) return 0;
        magnitude = magnitude * 10U + digit;
    }
    /* Keep the most-negative input representable without negating INT32_MIN. */
    if (negative && magnitude != 0U) *out = -(int32_t)(magnitude - 1U) - 1;
    else *out = (int32_t)magnitude;
    return 1;
}

static int compile_int_input_file(const char *path, const char *destination)
{
    uint8_t source[UMC_SOURCE_LIMIT];
    uint8_t encoded[UMC_BYTECODE_LIMIT];
    size_t length = 0U;
    size_t size = 0U;
    UmcDiagnostic diagnostic;
    if (!read_file(path, source, sizeof source, &length)) return 2;
    const UmcStatus status = umc_compile_int_input((const char *)source, length,
        encoded, sizeof encoded, &size, &diagnostic);
    if (status != UMC_OK) {
        umc_locate_diagnostic((const char *)source, length, &diagnostic);
        fprintf(stderr, "%s:%zu:%zu: %s\n", path, diagnostic.line, diagnostic.column, diagnostic.message);
        print_source_excerpt(source, length, diagnostic.source_offset);
        return 1;
    }
    if (!save_new_file(destination, encoded, size)) return 2;
    printf("Compiled a required-input artifact to %s\n", destination);
    return report_write_failed(stdout) ? 2 : 0;
}

static int execute_int_input_file(const char *path, int32_t input, size_t fuel)
{
    uint8_t encoded[UMC_BYTECODE_LIMIT];
    UmcInstruction program[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    UmcResult result;
    size_t size = 0U;
    size_t count = 0U;
    if (!read_file(path, encoded, sizeof encoded, &size)) return 2;
    UmcStatus status = umc_bind_int_input_bytecode(encoded, size, input, program,
        UMC_PROGRAM_LIMIT, &count, &diagnostic);
    if (status == UMC_OK) status = umc_execute(program, count, fuel, &result, &diagnostic);
    if (status != UMC_OK) {
        fprintf(stderr, "%s: source-byte=%zu: %s\n", path, diagnostic.source_offset, diagnostic.message);
        return 1;
    }
    printf("Result: %" PRId32 "\nInstructions: %zu\n", result.value, result.instructions_executed);
    return report_write_failed(stdout) ? 2 : 0;
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
    /* Retain the early stream check for review. The shared host helper also
     * observes errors raised when a buffered profile reaches its destination. */
#if 0
    return ferror(stdout) ? 1 : 0;
#endif
    return report_write_failed(stdout) ? 1 : 0;
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
            print_source_excerpt(source, bytes, diagnostic.source_offset);
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
    /* Retain the command-local flush for review. The shared host helper keeps
     * exactly this completion rule for reports from every command route. */
#if 0
    if (fflush(stdout) != 0 || ferror(stdout)) return 2;
#endif
    if (report_write_failed(stdout)) return 2;
    /* Deliberately distinguish a valid difference from invalid input. This
     * command-specific result must not be mistaken for general C equivalence,
     * matching diagnostic locations, binary identity or build-cache approval. */
    return equal ? 0 : 1;
}

/* Region reporting never treats a recognized language as executable support. */
static int print_regions(const char *name)
{
    uint8_t source[UMC_SOURCE_LIMIT];size_t length=0,count=0;
    UmcSourceRegion regions[UMC_REGION_LIMIT];UmcDiagnostic d;
    if(!read_file(name,source,sizeof source,&length))return 2;
    if(umc_source_regions((const char *)source,length,regions,UMC_REGION_LIMIT,&count,&d)!=UMC_OK){
        fprintf(stderr,"%s: source-byte=%zu: %s\n",name,d.source_offset,d.message);return 1;
    }
    for(size_t i=0;i<count;++i)printf("region=%zu language=%s offset=%zu bytes=%zu\n",i,
        umc_source_language_name(regions[i].language),regions[i].offset,regions[i].length);
/* Keep the previous classification disclaimer while naming the newly recognized D envelope. The former implementation is retained for owner review. */
#if 0
    puts("Classification only; tags do not compile or execute BASIC, Java or full C23.");
#endif
    puts("Classification only; tags do not compile or execute BASIC, Java, D or full C23.");
    return fflush(stdout)!=0 || ferror(stdout)?2:0;
}

/* Keep file I/O and presentation in the hosted command. The compiler owns
 * declaration tracking and byte positions; this route never invokes the VM. */
static int check_source_warnings(const char *name)
{
    uint8_t source[UMC_SOURCE_LIMIT];
    size_t length = 0U;
    UmcSourceReport report;
    UmcDiagnostic diagnostic;
    if (!read_file(name, source, sizeof source, &length)) return 2;
    if (umc_check_source((const char *)source, length, &report, &diagnostic) != UMC_OK) {
        fprintf(stderr, "%s:%zu:%zu: %s\n", name, diagnostic.line,
            diagnostic.column, diagnostic.message);
        print_source_excerpt(source, length, diagnostic.source_offset);
        return 2;
    }
    for (size_t i = 0U; i < report.warning_count; ++i) {
        const UmcSourceWarning *warning = &report.warnings[i];
        fprintf(stderr, "%s:%zu:%zu: warning: local '%.*s' is never read\n",
            name, warning->line, warning->column, (int)warning->source_length,
            (const char *)source + warning->source_offset);
        print_source_excerpt(source, length, warning->source_offset);
    }
    printf("Syntax and bytecode structure accepted: %zu instructions; %zu warnings\n",
        report.instruction_count, report.warning_count);
    /* Retain the earlier stream-state check for review. Finish both streams
     * so an output failure cannot hide buffered diagnostics. */
#if 0
    return ferror(stdout) || ferror(stderr) ? 1 : 0;
#endif
    const int output_failed = report_write_failed(stdout);
    const int diagnostics_failed = report_write_failed(stderr);
    return output_failed || diagnostics_failed ? 1 : 0;
}


int main(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "check") == 0 && strcmp(argv[3], "--warnings") == 0)
        return check_source_warnings(argv[2]);
    if (argc >= 2 && strcmp(argv[1], "regions") == 0)
        return argc == 3 ? print_regions(argv[2]) : usage();
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
        /* Keep the former exit for review. A version report is complete only
         * when the host has delivered its text. */
#if 0
        return 0;
#endif
        return report_write_failed(stdout) ? 2 : 0;
    }
    if (argc < 3) return usage();
    if (strcmp(argv[1], "compile-input") == 0)
        return argc == 4 ? compile_int_input_file(argv[2], argv[3]) : usage();
    if (strcmp(argv[1], "exec-input") == 0) {
        if (argc != 4 && argc != 6) return usage();
        int32_t input = 0;
        if (!parse_int_input(argv[3], &input)) {
            fputs("The input must be a signed 32-bit decimal integer.\n", stderr);
            return 2;
        }
        size_t fuel = UMC_PROGRAM_LIMIT;
        if (argc == 6 && (strcmp(argv[4], "--max-steps") != 0 || !step_limit(argv[5], &fuel))) {
            fprintf(stderr, "--max-steps requires a decimal integer from 1 to %u.\n",
                (unsigned)UMC_PROGRAM_LIMIT);
            return 2;
        }
        return execute_int_input_file(argv[2], input, fuel);
    }
    if (strcmp(argv[1], "translate") == 0) {
        if (argc != 5) return usage();
        UmcTranslationDirection direction;
        if (strcmp(argv[2], "c-to-java") == 0) direction = UMC_TRANSLATE_C_TO_JAVA;
        else if (strcmp(argv[2], "java-to-c") == 0) direction = UMC_TRANSLATE_JAVA_TO_C;
        else return usage();
        /* The compiler owns the portable subset and target output. The
         * command shares the same bounded reader and exclusive writer used
         * for checked Umicom C source and bytecode files. */
        uint8_t input[UMC_SOURCE_LIMIT];
        /* The extended API keeps the earlier expression mode internally.
         * Retain the old output extent here for the compatibility review. */
#if 0
        char output[UMC_TRANSLATION_OUTPUT_CAPACITY];
#endif
        char output[UMC_TRANSLATION_FUNCTION_OUTPUT_CAPACITY];
        size_t input_size;
        size_t output_size;
        UmcDiagnostic diagnostic;
        if (!read_file(argv[3], input, sizeof input, &input_size)) return 1;
        /* Retain the original call while the checked function profile adds
         * local statements through the same hosted command. */
#if 0
        const UmcStatus status = umc_translate_integer_answer(direction,
            (const char *)input, input_size, output, sizeof output, &output_size, &diagnostic);
#endif
        const UmcStatus status = umc_translate_integer_function(direction,
            (const char *)input, input_size, output, sizeof output, &output_size, &diagnostic);
        if (status != UMC_OK) {
            fprintf(stderr, "%zu:%zu: %s\n", diagnostic.line, diagnostic.column, diagnostic.message);
            return 1;
        }
        return save_new_file(argv[4], (const uint8_t *)output, output_size) ? 0 : 1;
    }
    const int check = strcmp(argv[1], "check") == 0;
    const int run = strcmp(argv[1], "run") == 0;
    const int compile = strcmp(argv[1], "compile") == 0;
    const int execute = strcmp(argv[1], "exec") == 0;
    const int disassemble = strcmp(argv[1], "disassemble") == 0;
    const int inspect_source = strcmp(argv[1], "inspect") == 0;
    const int inspect_bytes = strcmp(argv[1], "inspect-bytecode") == 0;
    const int budget_source = strcmp(argv[1], "budget") == 0;
    const int budget_bytes = strcmp(argv[1], "budget-bytecode") == 0;
    const int budgeting = budget_source || budget_bytes;
    const int trace_source = strcmp(argv[1], "trace") == 0;
    const int trace_bytes = strcmp(argv[1], "trace-bytecode") == 0;
    const int tracing = trace_source || trace_bytes;
    const int debug_source = strcmp(argv[1], "debug") == 0;
    const int debug_bytes = strcmp(argv[1], "debug-bytecode") == 0;
    const int debugging = debug_source || debug_bytes;
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
/* Admit interactive stepping through the same bounded compiler and bytecode routes. The former implementation is retained for owner review. */
#if 0
        !inspect_source && !inspect_bytes && !tracing) return usage();
    if ((run || execute || tracing) && argc == 5) {
#endif
/* Keep the prior command gate for review; path-budget commands add a read-only route. The former implementation is retained for owner review. */
#if 0
        !inspect_source && !inspect_bytes && !tracing && !debugging) return usage();
#endif
        !inspect_source && !inspect_bytes && !tracing && !debugging && !budgeting) return usage();
    if ((run || execute || tracing || debugging) && argc == 5) {
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
/* Keep bytecode debugger input on the bounded decoder. The former implementation is retained for owner review. */
#if 0
    const int binary = execute || disassemble || inspect_bytes || trace_bytes;
#endif
/* Read budget-bytecode through the existing bounded decoder; preserve the previous selector. The former implementation is retained for owner review. */
#if 0
    const int binary = execute || disassemble || inspect_bytes || trace_bytes || debug_bytes;
#endif
    const int binary = execute || disassemble || inspect_bytes || trace_bytes || debug_bytes || budget_bytes;
    if (!read_file(argv[2], input, binary ? UMC_BYTECODE_LIMIT : UMC_SOURCE_LIMIT, &length)) {
        return 2;
    }
    UmcStatus status = binary ?
        umc_decode(input, length, instructions, UMC_PROGRAM_LIMIT, &count, &diagnostic) :
        umc_compile((const char *)input, length, instructions, UMC_PROGRAM_LIMIT, &count, &diagnostic);
    if (status == UMC_OK && debugging)
        return umc_debug_console(binary ? NULL : (const char *)input, length, instructions, count, fuel, stdin, stdout, stderr);
    /* Keep the existing inspect output stable. Budgeting has its own additive
     * commands and reuses runtime validation without performing arithmetic. */
    if (status == UMC_OK && budgeting) {
        UmcExecutionBounds bounds;
        status = umc_execution_bounds(instructions, count, &bounds, &diagnostic);
        if (status == UMC_OK) {
            printf("Structural minimum instructions: %zu\nStructural maximum instructions: %zu\n",
                bounds.minimum_instructions, bounds.maximum_instructions);
            puts("Both branch alternatives are considered; arithmetic has not been executed.");
        }
    } else if (status == UMC_OK && (inspect_source || inspect_bytes)) {
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
        if (!binary) print_source_excerpt(input, length, diagnostic.source_offset);
        return 1;
    }
    /* Keep the previous successful execution exit for review. The published
     * report is part of successful CLI completion, including after compilation
     * has already closed the new output file. */
#if 0
    return 0;
#endif
    return report_write_failed(stdout) ? 2 : 0;
}
