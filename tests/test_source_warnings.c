/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_source_warnings.c
 *
 * PURPOSE:
 *   Verify lexical read diagnostics, scope reuse, refusal-preserving reports and parity with the existing compiler.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


#include "test.h"
#include "umicom_c/source_check.h"
#include "umicom_c/profile.h"
#include <stdarg.h>

static int check_source(const char *source, size_t expected)
{
    UmcSourceReport report;
    UmcInstruction code[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic;
    size_t count = 0U;
    CHECK(umc_check_source(source, strlen(source), &report, &diagnostic) == UMC_OK);
    CHECK(report.warning_count == expected && diagnostic.status == UMC_OK);
    CHECK(umc_compile(source, strlen(source), code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    CHECK(report.instruction_count == count);
    for (size_t i = 0U; i < report.warning_count; ++i) {
        const UmcSourceWarning *warning = &report.warnings[i];
        CHECK(warning->kind == UMC_WARNING_LOCAL_NEVER_READ);
        CHECK(warning->source_offset < strlen(source));
        CHECK(warning->source_length > 0U && warning->source_length <= UMC_IDENTIFIER_LIMIT);
        CHECK(warning->source_length <= strlen(source) - warning->source_offset);
        CHECK(warning->line > 0U && warning->column > 0U);
        if (i != 0U) CHECK(report.warnings[i - 1U].source_offset < warning->source_offset);
    }
    return 0;
}
static int reads(void)
{
    const struct { const char *source; size_t warnings; } cases[] = {
        {"int main(void){return 42;}",0},
        {"int main(void){int x=1;return 42;}",1},
        {"int main(void){const int x=1;return 42;}",1},
        {"int main(void){int x=1;x=2;return 42;}",1},
        {"int main(void){int x=1;return x;}",0},
        {"int main(void){int x=1;int y=x;return 42;}",1},
        {"int main(void){int x=1;x=x+1;return 42;}",0},
        {"int main(void){int x=1;int y=2;x=y;return 42;}",1},
        {"int main(void){int x=1;int y=2;return x+y;}",0}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        CHECK(check_source(cases[i].source, cases[i].warnings) == 0);
    return 0;
}
static int updates(void)
{
    const char *updates[] = {"x+=2;", "x-=2;", "x*=2;", "x/=2;", "x%=2;",
        "x&=2;", "x^=2;", "x|=2;", "x++;", "x--;", "++x;", "--x;"};
    for (size_t i = 0; i < sizeof updates / sizeof updates[0]; ++i) {
        char source[256];
        int n = snprintf(source, sizeof source, "int main(void){int x=1;%sreturn 42;}", updates[i]);
        CHECK(n > 0 && (size_t)n < sizeof source);
        CHECK(check_source(source, 0U) == 0);
    }
    return 0;
}
static int branches(void)
{
    const char *sources[] = {
        "int main(void){int x=1;if(0){int y=x;}return 42;}",
        "int main(void){int x=1;int y=0&&x;return 42;}",
        "int main(void){int x=1;int y=1||x;return 42;}",
        "int main(void){int x=1;int y=1?42:x;return 42;}",
        "int main(void){int x=1;if(x){int y=2;}return 42;}"
    };
    for (size_t i = 0; i < sizeof sources / sizeof sources[0]; ++i)
        CHECK(check_source(sources[i], 1U) == 0);
    CHECK(check_source("int main(void){int x=1;if(0){x=2;}return 42;}",1U) == 0);
    CHECK(check_source("int main(void){int x=1;if(0){x++;}return 42;}",0U) == 0);
    return 0;
}
static int scopes(void)
{
    const char *source = "int main(void){int outer=1;{int same=2;}{int same=3;outer=same;}return outer;}";
    UmcSourceReport report;
    CHECK(umc_check_source(source, strlen(source), &report, NULL) == UMC_OK);
    CHECK(report.warning_count == 1U);
    CHECK(report.warnings[0].source_offset == (size_t)(strstr(source,"same=2")-source));
    const char *ordered = "int main(void){int outer=1;{int inner=2;{int deep=3;}}return 42;}";
    CHECK(umc_check_source(ordered, strlen(ordered), &report, NULL) == UMC_OK);
    CHECK(report.warning_count == 3U);
    CHECK(strncmp(ordered+report.warnings[0].source_offset,"outer",5U) == 0);
    CHECK(strncmp(ordered+report.warnings[1].source_offset,"inner",5U) == 0);
    CHECK(strncmp(ordered+report.warnings[2].source_offset,"deep",4U) == 0);
    CHECK(check_source("int main(void){if(1){int x=1;}else{int x=2;}return 42;}",2U) == 0);
    CHECK(check_source("int main(void){{int x=1;x++;}{int x=2;}return 42;}",1U) == 0);
    CHECK(check_source("int main(void){{int x=1;}{int x=2;x++;}return 42;}",1U) == 0);
    return 0;
}
static int positions(void)
{
    const char *endings[] = {"\n", "\r\n", "\r"};
    for (size_t i = 0; i < 3U; ++i) {
        char source[256];
        int n = snprintf(source,sizeof source,"int main(void){%s\tint unused=7;%sreturn 42;}",endings[i],endings[i]);
        CHECK(n > 0 && (size_t)n < sizeof source);
        UmcSourceReport report;
        CHECK(umc_check_source(source,(size_t)n,&report,NULL) == UMC_OK);
        CHECK(report.warning_count == 1U);
        CHECK(report.warnings[0].line == 2U && report.warnings[0].column == 6U);
        CHECK(report.warnings[0].source_offset == (size_t)(strstr(source,"unused")-source));
        CHECK(report.warnings[0].source_length == 6U);
    }
    return 0;
}
static int refusal(const char *source, size_t length, UmcStatus expected)
{
    UmcSourceReport report, before;
    memset(&report,0xA5,sizeof report);
    memcpy(&before,&report,sizeof report);
    UmcDiagnostic diagnostic;
    CHECK(umc_check_source(source,length,&report,&diagnostic) == expected);
    CHECK(diagnostic.status == expected);
    CHECK(memcmp(&report,&before,sizeof report) == 0);
    return 0;
}
static int refusals(void)
{
    const char *sources[] = {
        "int main(void){int x=1;return missing;}",
        "int main(void){int x=1;return 42;}trailing",
        "int main(void){int x=1;int x=2;return 42;}",
        "int main(void){int x=1;{int x=2;}return 42;}",
        "int main(void){const int x=1;x++;return 42;}",
        "int main(void){int x=1;{int y=2;}return y;}"
    };
    for (size_t i = 0; i < sizeof sources / sizeof sources[0]; ++i)
        CHECK(refusal(sources[i],strlen(sources[i]),UMC_SYNTAX_ERROR) == 0);
    CHECK(refusal(NULL,0U,UMC_INVALID_ARGUMENT) == 0);
    CHECK(refusal("",SIZE_MAX,UMC_SOURCE_TOO_LARGE) == 0);
    UmcDiagnostic diagnostic;
    CHECK(umc_check_source("",0U,NULL,&diagnostic) == UMC_INVALID_ARGUMENT);
    CHECK(diagnostic.status == UMC_INVALID_ARGUMENT);
    return 0;
}
static int append(char *text, size_t capacity, size_t *used, const char *format, ...)
{
    va_list arguments; va_start(arguments,format);
    int n = vsnprintf(text+*used,capacity-*used,format,arguments);
    va_end(arguments);
    if (n < 0 || (size_t)n >= capacity-*used) return 1;
    *used += (size_t)n;
    return 0;
}
static int limits(void)
{
    char source[8192]="int main(void){";
    size_t used = strlen(source);
    /* 127 independent declarations consume 254 instructions; RETURN 0 uses
     * the last two. Distinct declaration identities survive VM slot reuse. */
    for (size_t i = 0; i < 127U; ++i) CHECK(!append(source,sizeof source,&used,"{int x=1;}"));
    CHECK(!append(source,sizeof source,&used,"return 0;}"));
    UmcSourceReport report;
    CHECK(umc_check_source(source,used,&report,NULL) == UMC_OK);
    CHECK(report.instruction_count == 256U && report.warning_count == 127U);
    CHECK(check_source(source,127U) == 0);
    used = 0; source[0] = '\0';
    CHECK(!append(source,sizeof source,&used,"int main(void){"));
    for (size_t i = 0; i < 128U; ++i) CHECK(!append(source,sizeof source,&used,"{int x=1;}"));
    CHECK(!append(source,sizeof source,&used,"return 0;}"));
    CHECK(refusal(source,used,UMC_PROGRAM_TOO_LARGE) == 0);
    used = 0; source[0] = '\0';
    CHECK(!append(source,sizeof source,&used,"int main(void){"));
    for (size_t i = 0; i < 33U; ++i) CHECK(!append(source,sizeof source,&used,"int x%zu=1;",i));
    CHECK(!append(source,sizeof source,&used,"return 0;}"));
    CHECK(refusal(source,used,UMC_LOCAL_CAPACITY) == 0);
    return 0;
}
static int spans(void)
{
    const char source[] = "int main(void){int x=1;return 42;}!";
    const size_t admitted = sizeof source - 2U;
    UmcSourceReport report;
    CHECK(umc_check_source(source,admitted,&report,NULL) == UMC_OK);
    CHECK(report.warning_count == 1U);
    for (size_t n = 0U; n < admitted; ++n) {
        UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count;
        UmcDiagnostic expected, actual;
        UmcStatus status = umc_compile(source,n,code,UMC_PROGRAM_LIMIT,&count,&expected);
        CHECK(status != UMC_OK);
        memset(&report,0x5A,sizeof report);
        UmcSourceReport before; memcpy(&before,&report,sizeof report);
        CHECK(umc_check_source(source,n,&report,&actual) == status);
        CHECK(actual.source_offset == expected.source_offset && actual.line == expected.line && actual.column == expected.column);
        CHECK(memcmp(&report,&before,sizeof report) == 0);
    }
    CHECK(refusal(source,sizeof source-1U,UMC_SYNTAX_ERROR) == 0);
    return 0;
}
static int no_execution(void)
{
    const char *sources[] = {
        "int main(void){int x=1/0;return 42;}",
        "int main(void){int x=2147483647+1;return 42;}"
    };
    for (size_t i = 0; i < 2U; ++i) {
        CHECK(check_source(sources[i],1U) == 0);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; size_t count;
        UmcResult result; UmcDiagnostic diagnostic;
        CHECK(umc_compile(sources[i],strlen(sources[i]),code,UMC_PROGRAM_LIMIT,&count,&diagnostic) == UMC_OK);
        CHECK(umc_execute(code,count,count,&result,&diagnostic) == UMC_ARITHMETIC_ERROR);
    }
    return 0;
}
static int repeat(void)
{
    char source[]="int main(void){int unused=1;return 42;}";
    UmcSourceReport first, report;
    CHECK(umc_check_source(source,strlen(source),&first,NULL) == UMC_OK);
    for (size_t i=0; i<50U; ++i) {
        CHECK(check_source("int main(void){int used=1;return used;}",0U) == 0);
        CHECK(umc_check_source(source,strlen(source),&report,NULL) == UMC_OK);
        CHECK(report.warning_count == first.warning_count);
        CHECK(report.warnings[0].source_offset == first.warnings[0].source_offset);
        CHECK(report.warnings[0].line == first.warnings[0].line);
    }
    source[0]='!'; /* The report owns numeric positions; no borrowed text. */
    CHECK(report.warnings[0].source_length == 6U);
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES) & UMC_FEATURE_SOURCE_WARNINGS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES) & UMC_FEATURE_SOURCE_WARNINGS));
    return 0;
}
int main(int argc, char **argv)
{
    if(argc != 2) return 2;
    const struct {const char *name; int (*run)(void);} cases[] = {
        {"reads",reads},{"updates",updates},{"branches",branches},{"scopes",scopes},
        {"positions",positions},{"refusals",refusals},{"limits",limits},
        {"spans",spans},{"no-execution",no_execution},{"repeat",repeat}
    };
    for(size_t i=0; i<sizeof cases/sizeof cases[0]; ++i)
        if(strcmp(argv[1],cases[i].name)==0) return cases[i].run();
    return 2;
}
