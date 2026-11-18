/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_boolean_literals.c
 *
 * PURPOSE:
 *   Verify exact Boolean keywords, native expression parity, bounded source access and unchanged integer wire semantics.
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

static int value(const char *source, int32_t expected)
{
    UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; UmcResult r;
    size_t count = 0;
    CHECK(umc_compile(source, strlen(source), code, UMC_PROGRAM_LIMIT, &count, &d) == UMC_OK);
    CHECK(umc_execute(code, count, count, &r, &d) == UMC_OK && r.value == expected);
    return 0;
}
static int values(void)
{
    CHECK(value("int main(void){return true;}", 1) == 0);
    CHECK(value("int main(void){return false;}", 0) == 0);
    CHECK(value("int main(void){return -true;}", -1) == 0);
    CHECK(value("int main(void){return true+true;}", 2) == 0);
    CHECK(value("int main(void){return ~false;}", -1) == 0);
    CHECK(value("int main(void){return false ? 1/0 : 42;}", 42) == 0);
    CHECK(value("int main(void){return true ? 42 : 1/0;}", 42) == 0);
    CHECK(value("int main(void){return false && (1/0);}", 0) == 0);
    CHECK(value("int main(void){return true || (1/0);}", 1) == 0);
    CHECK(value("int main(void){return true /* comment */ + false;}", 1) == 0);
    return 0;
}
static int native_parity(void)
{
    /* Each expected value is evaluated by the host C compiler, independently
     * of Umicom's lexer and VM. This covers only the common integer expressions. */
#define NATIVE_CASE(expression) { "int main(void){return " #expression ";}", (int32_t)(expression) }
    static const struct { const char *source; int32_t expected; } cases[] = {
        NATIVE_CASE(true), NATIVE_CASE(false), NATIVE_CASE(!true), NATIVE_CASE(!false),
        NATIVE_CASE(true + false * 7), NATIVE_CASE(true - true), NATIVE_CASE(true << 4),
        NATIVE_CASE(true == false), NATIVE_CASE(true != false), NATIVE_CASE(true > false),
        NATIVE_CASE(true && false), NATIVE_CASE(false || true),
        NATIVE_CASE(true ? 42 : 3), NATIVE_CASE(false ? 3 : 42),
        NATIVE_CASE(true ? false ? 3 : 42 : 9)
    };
#undef NATIVE_CASE
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        CHECK(value(cases[i].source, cases[i].expected) == 0);
    return 0;
}
static int keywords(void)
{
    CHECK(value("int main(void){int true_value=6,falsehood=7;return true_value*falsehood;}", 42) == 0);
    CHECK(value("int main(void){int true0=40,false_=2;return true0+false_;}", 42) == 0);
    CHECK(value("int main(void){int True=40,FALSE=2;return True+FALSE;}", 42) == 0);
    const char *invalid[] = {
        "int main(void){int true=1;return true;}",
        "int main(void){const int false=0;return false;}",
        "int main(void){true=0;return true;}",
        "int main(void){false++;return false;}",
        "int main(void){++true;return true;}",
        "int main(void){bool ready=true;return ready;}",
        "int main(void){return true false;}",
        "int main(void){return true(1);}",
        "int main(void){return true'0;}",
        "int main(void){return tru;}",
        "int main(void){return False;}"
    };
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        UmcInstruction code[32]; UmcDiagnostic d; size_t count = 71;
        CHECK(umc_compile(invalid[i], strlen(invalid[i]), code, 32, &count, &d) != UMC_OK);
        CHECK(count == 0);
    }
    return 0;
}
static int scope(void)
{
    CHECK(value("int main(void){const int enabled=true;int n=false;if(enabled){int amount=42;n=amount;}return n;}",42)==0);
    CHECK(value("int main(void){int r=0;if(false){r=1/0;}else if(true){r=42;}else{r=9;}return r;}",42)==0);
    CHECK(value("int main(void){int n=true;n+=true;n*=21;return n;}",42)==0);
    CHECK(value("int main(void){int r=false;{int trueValue=42;r=trueValue;}return r;}",42)==0);
    /* A skipped branch still has to be syntactically valid and names bound. */
    const char *source="int main(void){return false && missing;}";
    UmcInstruction code[32];UmcDiagnostic d;size_t count=71;
    CHECK(umc_compile(source,strlen(source),code,32,&count,&d)!=UMC_OK && count==0);
    return 0;
}
static int warnings(void)
{
    const char *source="int main(void){int unused=true;return false;}";
    UmcSourceReport report, before; UmcDiagnostic d;
    CHECK(umc_check_source(source,strlen(source),&report,&d)==UMC_OK);
    CHECK(report.warning_count==1 && report.warnings[0].kind==UMC_WARNING_LOCAL_NEVER_READ);
    CHECK(report.warnings[0].source_offset==19 && report.warnings[0].source_length==6);
    source="int main(void){int flag=true;return false && flag;}";
    CHECK(umc_check_source(source,strlen(source),&report,&d)==UMC_OK && report.warning_count==0);
    memset(&report,0x5a,sizeof report);memcpy(&before,&report,sizeof before);
    source="int main(void){int true=1;return false;}";
    CHECK(umc_check_source(source,strlen(source),&report,&d)!=UMC_OK);
    CHECK(memcmp(&report,&before,sizeof report)==0);
    return 0;
}
static int spans(void)
{
    const char *sources[]={"int main(void){return true;}","int main(void){return false;}",
        "int main(void){return false || true;}"};
    for(size_t i=0;i<sizeof sources/sizeof sources[0];++i){
        char exact[80];size_t length=strlen(sources[i]);memcpy(exact,sources[i],length);
        UmcInstruction code[32];UmcDiagnostic d;size_t count=0;
        CHECK(umc_compile(exact,length,code,32,&count,&d)==UMC_OK);
        for(size_t end=0;end<length;++end)
            CHECK(umc_compile(exact,end,code,32,&count,&d)!=UMC_OK && count==0);
    }
    char source[]="int main(void){return true;}";source[24]='\0';
    UmcInstruction code[32];UmcDiagnostic d;size_t count=71;
    CHECK(umc_compile(source,sizeof source-1,code,32,&count,&d)!=UMC_OK && count==0);
    const char *bad="int main(void){\r\nint true=1;return 0;}";
    CHECK(umc_compile(bad,strlen(bad),code,32,&count,&d)==UMC_SYNTAX_ERROR);
    CHECK(d.line==2 && d.column==5);
    return 0;
}
static int wire(void)
{
    const char *sources[]={"int main(void){return true;}","int main(void){return false;}"};
    for(size_t i=0;i<2;++i){
        UmcInstruction code[4],decoded[4];UmcDiagnostic d;UmcResult r;
        size_t count=0,used=0,decodedCount=0;uint8_t bytes[UMC_BYTECODE_LIMIT];
        CHECK(umc_compile(sources[i],strlen(sources[i]),code,4,&count,&d)==UMC_OK && count==2);
        CHECK(code[0].opcode==UMC_PUSH && code[0].operand==(i==0?1:0) && code[0].source_offset==22);
        CHECK(umc_encode(code,count,bytes,sizeof bytes,&used,&d)==UMC_OK && bytes[4]==1);
        CHECK(umc_decode(bytes,used,decoded,4,&decodedCount,&d)==UMC_OK && decodedCount==2);
        CHECK(decoded[0].operand==code[0].operand && decoded[0].source_offset==22);
        CHECK(umc_execute(decoded,decodedCount,2,&r,&d)==UMC_OK && r.value==code[0].operand);
    }
    return 0;
}
static int fuel(void)
{
    const char *source="int main(void){return true;}";
    UmcInstruction code[4];UmcDiagnostic d;UmcResult r={71,73};size_t count=0;
    CHECK(umc_compile(source,strlen(source),code,4,&count,&d)==UMC_OK && count==2);
    CHECK(umc_execute(code,count,1,&r,&d)==UMC_FUEL_EXHAUSTED);
    CHECK(r.value==71 && r.instructions_executed==73);
    CHECK(umc_execute(code,count,2,&r,&d)==UMC_OK && r.value==1 && r.instructions_executed==2);
    CHECK(umc_compile(source,strlen(source),code,1,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0);
    return 0;
}
static int faults(void)
{
    const char *sources[]={"int main(void){return true/false;}","int main(void){return true%false;}",
        "int main(void){return true && (1/false);}"};
    for(size_t i=0;i<sizeof sources/sizeof sources[0];++i){
        UmcInstruction code[32];UmcDiagnostic d;UmcResult r={71,73};size_t count=0;
        CHECK(umc_compile(sources[i],strlen(sources[i]),code,32,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==71 && r.instructions_executed==73);
    }
    return 0;
}
int main(int argc,char **argv)
{
    CHECK(argc==2 && UMC_BOOLEAN_LITERAL_PROFILE==1U);
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_BOOLEAN_LITERALS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_BOOLEAN_LITERALS));
    if(!strcmp(argv[1],"values"))return values();
    if(!strcmp(argv[1],"native-parity"))return native_parity();
    if(!strcmp(argv[1],"keywords"))return keywords();
    if(!strcmp(argv[1],"scope"))return scope();
    if(!strcmp(argv[1],"warnings"))return warnings();
    if(!strcmp(argv[1],"spans"))return spans();
    if(!strcmp(argv[1],"wire"))return wire();
    if(!strcmp(argv[1],"fuel"))return fuel();
    if(!strcmp(argv[1],"faults"))return faults();
    return 2;
}
