/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_empty_statements.c
 *
 * PURPOSE:
 *   Verify empty-statement admission, zero instruction cost, diagnostics and unchanged runtime ownership.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


#include "test.h"
#include "umicom_c/profile.h"
#include "umicom_c/source_check.h"

static UmcStatus compile_text(const char *source, UmcInstruction *code, size_t capacity,
    size_t *count, UmcDiagnostic *diagnostic)
{
    return umc_compile(source, strlen(source), code, capacity, count, diagnostic);
}
static int values(void)
{
    const char *sources[] = {
        "int main(void){;return 42;}",
        "int main(void){;;;return 42;;;;}",
        "int main(void){{;}return 42;}",
        "int main(void){;{;{;;};};return 42;;}",
        "int main(void){;int x=41;;;x++;;return x;;}",
        "int main(void){int x=42;if(1){;}else{;;}return x;}",
        "int main(void){int x=0;if(0){;}else if(1){;x=42;;}else{;}return x;}",
        "int main(void){;int x=0;{;const int y=42;;x=y;;};return x;;}"
    };
    for(size_t i=0;i<sizeof sources/sizeof sources[0];++i){
        UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic diagnostic;UmcResult result;
        CHECK(compile_text(sources[i],code,UMC_PROGRAM_LIMIT,&count,&diagnostic)==UMC_OK);
        CHECK(umc_execute(code,count,count,&result,&diagnostic)==UMC_OK && result.value==42);
    }
    /* These exact bodies formerly appeared in the if/else-if refusal suites.
     * Admit them without changing the surrounding local value. */
    const char *formerRefusals[] = {
        "int main(void){int x=1;if(1){;}return x;}",
        "int main(void){int x=1;if(0){}else if(1){;}return x;}"
    };
    for (size_t i=0;i<sizeof formerRefusals/sizeof formerRefusals[0];++i) {
        UmcInstruction code[32];size_t count=0;UmcDiagnostic diagnostic;UmcResult result;
        CHECK(compile_text(formerRefusals[i],code,32,&count,&diagnostic)==UMC_OK);
        CHECK(umc_execute(code,count,count,&result,&diagnostic)==UMC_OK && result.value==1);
    }
    return 0;
}
static int cost(void)
{
    const char *plain="int main(void){int x=7;return x*6;}";
    const char *empty="int main(void){;;;int x=7;;;{;;}return x*6;;;;}";
    UmcInstruction a[32],b[32];size_t na=0,nb=0;UmcDiagnostic diagnostic;UmcResult result;
    CHECK(compile_text(plain,a,32,&na,&diagnostic)==UMC_OK);
    CHECK(compile_text(empty,b,32,&nb,&diagnostic)==UMC_OK && na==nb);
    for(size_t i=0;i<na;++i)CHECK(a[i].opcode==b[i].opcode && a[i].operand==b[i].operand);
    for(size_t fuel=0;fuel<=nb;++fuel){
        result=(UmcResult){73,91};
        UmcStatus status=umc_execute(b,nb,fuel,&result,&diagnostic);
        if(fuel<nb)CHECK(status==UMC_FUEL_EXHAUSTED && result.value==73 && result.instructions_executed==91U);
        else CHECK(status==UMC_OK && result.value==42 && result.instructions_executed==nb);
    }
    return 0;
}
static int wire(void)
{
    const char *source="int main(void){;{;;};return 6*7;;; }";
    UmcInstruction code[16],decoded[16];size_t count=0,bytes=0,decodedCount=0;
    uint8_t wireBytes[UMC_BYTECODE_LIMIT];UmcDiagnostic diagnostic;UmcResult result;
    CHECK(compile_text(source,code,16,&count,&diagnostic)==UMC_OK && count==4U);
    CHECK(umc_encode(code,count,wireBytes,sizeof wireBytes,&bytes,&diagnostic)==UMC_OK);
    CHECK(wireBytes[4]==1U);
    CHECK(umc_decode(wireBytes,bytes,decoded,16,&decodedCount,&diagnostic)==UMC_OK && decodedCount==count);
    for(size_t i=0;i<count;++i)CHECK(code[i].opcode==decoded[i].opcode &&
        code[i].operand==decoded[i].operand && code[i].source_offset==decoded[i].source_offset);
    CHECK(umc_execute(decoded,decodedCount,4,&result,&diagnostic)==UMC_OK && result.value==42);
    return 0;
}
static int refusals(void)
{
    const char *sources[]={
        "int main(void){;;;}", "int main(void){;return;}",
        "int main(void){;return 42;;int x=1;}", "int main(void){;return 42;;return 1;}",
        "int main(void){if(1);return 42;}", "int main(void){if(1){};else{}return 42;}",
        "int main(void){;1+2;return 42;}", "int main(void){{;return 42;};return 0;}",
        "int main(void){;return 42;};", "int main(void){;{int x=1;;};return x;}"
    };
    for(size_t i=0;i<sizeof sources/sizeof sources[0];++i){
        UmcInstruction code[32];size_t count=91;UmcDiagnostic diagnostic;
        CHECK(compile_text(sources[i],code,32,&count,&diagnostic)==UMC_SYNTAX_ERROR && count==0U);
    }
    return 0;
}
static int many(void)
{
    char source[8192];const char *prefix="int main(void){";
    size_t used=strlen(prefix);memcpy(source,prefix,used);
    memset(source+used,';',4096);used+=4096;
    const char *suffix="return 42;;;;}";memcpy(source+used,suffix,strlen(suffix)+1U);
    UmcInstruction code[2];size_t count=0;UmcDiagnostic diagnostic;UmcResult result;
    CHECK(compile_text(source,code,2,&count,&diagnostic)==UMC_OK && count==2U);
    CHECK(umc_execute(code,count,2,&result,&diagnostic)==UMC_OK && result.value==42);
    CHECK(compile_text(source,code,1,&count,&diagnostic)==UMC_PROGRAM_TOO_LARGE && count==0U);
    CHECK(umc_compile(source,UMC_SOURCE_LIMIT+1U,code,2,&count,&diagnostic)==UMC_SOURCE_TOO_LARGE);
    return 0;
}
static int warnings(void)
{
    const char *source="int main(void){;\r\nint unused=1;;;\r\nreturn 42;;;;}";
    UmcSourceReport report;UmcDiagnostic diagnostic;
    CHECK(umc_check_source(source,strlen(source),&report,&diagnostic)==UMC_OK);
    CHECK(report.instruction_count==4U && report.warning_count==1U);
    CHECK(report.warnings[0].kind==UMC_WARNING_LOCAL_NEVER_READ);
    CHECK(report.warnings[0].source_offset==(size_t)(strstr(source,"unused")-source));
    CHECK(report.warnings[0].line==2U && report.warnings[0].column==5U);
    return 0;
}
static int spans(void)
{
    const char source[]="int main(void){;{;;};return 42;;; }";
    char exact[sizeof source-1U];memcpy(exact,source,sizeof exact);
    UmcInstruction code[16];size_t count=0;UmcDiagnostic diagnostic;
    CHECK(umc_compile(exact,sizeof exact,code,16,&count,&diagnostic)==UMC_OK);
    for(size_t n=0;n<sizeof exact;++n)
        CHECK(umc_compile(exact,n,code,16,&count,&diagnostic)!=UMC_OK && count==0U);
    const char bad[]="int main(void){;;;\r\nreturn ;;;}";
    CHECK(umc_compile(bad,sizeof bad-1U,code,16,&count,&diagnostic)==UMC_SYNTAX_ERROR);
    CHECK(diagnostic.line==2U && diagnostic.column==8U);
    return 0;
}
int main(int argc,char **argv)
{
    CHECK(argc==2 && UMC_EMPTY_STATEMENT_PROFILE==1U);
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_EMPTY_STATEMENTS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_EMPTY_STATEMENTS));
    if(!strcmp(argv[1],"values"))return values();
    if(!strcmp(argv[1],"cost"))return cost();
    if(!strcmp(argv[1],"wire"))return wire();
    if(!strcmp(argv[1],"refusals"))return refusals();
    if(!strcmp(argv[1],"many"))return many();
    if(!strcmp(argv[1],"warnings"))return warnings();
    if(!strcmp(argv[1],"spans"))return spans();
    return 2;
}
