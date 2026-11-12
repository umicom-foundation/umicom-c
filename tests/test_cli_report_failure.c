/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_cli_report_failure.c
 *
 * PURPOSE:
 *   Exercise the real CLI entry with a buffered report whose operating-system descriptor refuses writes.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
/* Compile the real hosted entry, including its explicit report flush. The
 * fixture changes only the output descriptor; it does not mock stdio errors. */
#define main umicom_cli_test_entry
#include "../tools/main.c"
#undef main
#ifdef _WIN32
#include <io.h>
#define UMC_REPORT_FILENO _fileno
#define UMC_REPORT_DUP2 _dup2
#else
#include <unistd.h>
#define UMC_REPORT_FILENO fileno
#define UMC_REPORT_DUP2 dup2
#endif

#define REQUIRE_REPORT(x) do { if (!(x)) { \
    fprintf(stderr, "report fixture failed at line %d: %s\n", __LINE__, #x); return 1; \
} } while (0)

int main(int argc, char **argv)
{
    REQUIRE_REPORT(argc == 5);
    REQUIRE_REPORT(!strcmp(argv[3], "same") || !strcmp(argv[3], "different"));
    REQUIRE_REPORT(!strcmp(argv[4], "deny") || !strcmp(argv[4], "control"));
    const int semantic = strcmp(argv[3], "same") ? 1 : 0;
    const int deny = !strcmp(argv[4], "deny");
    uint8_t left_before[512], right_before[512], after[512];
    size_t left_size = 0U, right_size = 0U, after_size = 0U;
    REQUIRE_REPORT(read_file(argv[1], left_before, sizeof(left_before), &left_size));
    REQUIRE_REPORT(read_file(argv[2], right_before, sizeof(right_before), &right_size));
    /* Keep this buffer alive until stdout is closed. A short printf can now
     * succeed in memory; only the command's flush observes the denied write. */
    char output_buffer[4096];
    REQUIRE_REPORT(setvbuf(stdout, output_buffer, _IOFBF, sizeof(output_buffer)) == 0);
    FILE *read_only = NULL;
    if (deny) {
        read_only = fopen(argv[1], "rb");
        REQUIRE_REPORT(read_only != NULL);
        REQUIRE_REPORT(UMC_REPORT_DUP2(UMC_REPORT_FILENO(read_only), UMC_REPORT_FILENO(stdout)) >= 0);
    }
    char *arguments[] = {"umicom-c", "compare-operations", argv[1], argv[2], NULL};
    const int result = umicom_cli_test_entry(4, arguments);
    const int output_failed = ferror(stdout) != 0;
    /* Close while the denied descriptor is still installed. Restoring stdout
     * first could publish a failed report during process-exit cleanup. */
    const int closed = fclose(stdout);
    if (read_only) REQUIRE_REPORT(fclose(read_only) == 0);
    REQUIRE_REPORT(result == (deny ? 2 : semantic));
    REQUIRE_REPORT(output_failed == deny);
    if (!deny) REQUIRE_REPORT(closed == 0);
    REQUIRE_REPORT(read_file(argv[1], after, sizeof(after), &after_size));
    REQUIRE_REPORT(after_size == left_size && !memcmp(after, left_before, left_size));
    REQUIRE_REPORT(read_file(argv[2], after, sizeof(after), &after_size));
    REQUIRE_REPORT(after_size == right_size && !memcmp(after, right_before, right_size));
    fprintf(stderr, "PASS report %s %s: input bytes unchanged\n", argv[3], argv[4]);
    return 0;
}
