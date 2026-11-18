/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_basic_comment_regions.c
 *
 * PURPOSE:
 *   Check BASIC REM comment boundaries without treating region discovery as a foreign-language compiler.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


#include "test.h"
#include "umicom_c/source_regions.h"

static int single_basic(const char *source)
{
    UmcSourceRegion regions[4];
    size_t count = 99U;
    UmcDiagnostic diagnostic;
    CHECK(umc_source_regions(source, strlen(source), regions, 4U, &count, &diagnostic) == UMC_OK);
    CHECK(count == 1U && regions[0].language == UMC_LANGUAGE_BASIC);
    CHECK(regions[0].tag_offset == 0U && regions[0].offset == 7U);
    CHECK(regions[0].offset + regions[0].length == strlen(source) - 2U);
    return 0;
}
int main(int argc, char **argv)
{
    CHECK(argc == 2);
    if (strcmp(argv[1], "line-comments") == 0) {
        CHECK(single_basic("<?basic\nREM ?> <?java ignored\nPRINT 1\n?>") == 0);
        CHECK(single_basic("<?basic\r\n  rEm\t?> <?basic ignored\r\nPRINT 1\r\n?>") == 0);
        CHECK(single_basic("<?basic REM ?> ignored\r?>") == 0);
    } else if (strcmp(argv[1], "colon-comments") == 0) {
        CHECK(single_basic("<?basic\nPRINT 1 : REM <?java ?> ignored\n?>") == 0);
        CHECK(single_basic("<?basic\nPRINT \"a: REM ?>\": rEm ?> ignored\n?>") == 0);
        CHECK(single_basic("<?basic\n: : rem ?> ignored\n?>") == 0);
    } else if (strcmp(argv[1], "keyword-boundaries") == 0) {
        CHECK(single_basic("<?basic\nREMEMBER = 1 ?>") == 0);
        CHECK(single_basic("<?basic\nrem_value = 1 ?>") == 0);
        CHECK(single_basic("<?basic\nREM$ = 1 ?>") == 0);
        CHECK(single_basic("<?basic\nPRINT REM ?>") == 0);
        CHECK(single_basic("<?basic\nREM?>") == 0);
    } else if (strcmp(argv[1], "quotes-and-comments") == 0) {
        CHECK(single_basic("<?basic\nPRINT \"REM ?>\"\n?>") == 0);
        CHECK(single_basic("<?basic\nPRINT \"a\"\"REM ?>b\" ' REM <?java ?>\n?>") == 0);
        CHECK(single_basic("<?basic\nREM \"an unmatched quote <?java ?>\nPRINT 1\n?>") == 0);
    } else if (strcmp(argv[1], "other-languages") == 0) {
        UmcSourceRegion regions[4]; size_t count; UmcDiagnostic diagnostic;
        const char *java = "<?java\nREM ?>";
        CHECK(umc_source_regions(java, strlen(java), regions, 4U, &count, &diagnostic) == UMC_OK);
        CHECK(count == 1U && regions[0].language == UMC_LANGUAGE_JAVA);
        const char *plain = "REM ?>";
        CHECK(umc_source_regions(plain, strlen(plain), regions, 4U, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        CHECK(count == 0U && diagnostic.source_offset == 4U);
    } else if (strcmp(argv[1], "bounded-span") == 0) {
        const char exact[] = {'<','?','b','a','s','i','c',' ','R','E','M'};
        UmcSourceRegion regions[4]; size_t count = 99U; UmcDiagnostic diagnostic;
        CHECK(umc_source_regions(exact, sizeof(exact), regions, 4U, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        CHECK(count == 0U && diagnostic.source_offset == 0U);
        CHECK(umc_source_regions(exact, sizeof(exact) - 1U, regions, 4U, &count, &diagnostic) == UMC_SYNTAX_ERROR);
        CHECK(count == 0U && diagnostic.source_offset == 0U);
        const char *source = "<?basic\nREM ?> ignored\n?>outside";
        const size_t admitted = strlen(source) - strlen("outside");
        CHECK(umc_source_regions(source, admitted, regions, 4U, &count, &diagnostic) == UMC_OK);
        CHECK(count == 1U && regions[0].offset + regions[0].length == admitted - 2U);
        CHECK(umc_source_regions(source, admitted, regions, 0U, &count, &diagnostic) == UMC_BUFFER_TOO_SMALL);
        CHECK(count == 0U);
    } else return 2;
    return 0;
}
