/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_declarator_lists.c
 *
 * PURPOSE:
 *   Qualify initialized declaration lists without changing local lifetime, warnings or runtime instructions.
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
static int value(const char *source,int32_t expected)
{
    UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic d;UmcResult r;
    CHECK(umc_compile(source,strlen(source),code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,UMC_PROGRAM_LIMIT,&r,&d)==UMC_OK && r.value==expected);
    return 0;
}
static int values(void)
{
    CHECK(value("int main(void){int first=6,second=first*7;return second;}",42)==0);
    CHECK(value("int main(void){int a=1,b=a+2,c=b*3;return a+b+c;}",13)==0);
    CHECK(value("int main(void){int a=0?1:2,b=a?4:8;return b;}",4)==0);
    CHECK(value("int main(void){int a=',',b='A';a+=b;return a;}",109)==0);
    CHECK(value("int main(void){int a=6,/* separate */ b=a*7;return b;}",42)==0);
    return 0;
}
static int scope(void)
{
    CHECK(value("int main(void){int result=0;{int a=2,b=a*3;result=b;}{int a=3,b=a+4;result+=b;}return result;}",13)==0);
    CHECK(value("int main(void){int result=0;if(0){int a=1,b=1/0;result=a+b;}else{int a=6,b=a*7;result=b;}return result;}",42)==0);
    /* A success helper must not be inverted to test an expected compile error.
     * Keep that draft fixture for review; the refusal table owns invalid input. */
#if 0
    CHECK(value("int main(void){const int base=6;{int a=base,b=a*7;return_value=1;}return base;}",6)!=0);
#endif
    CHECK(value("int main(void){const int base=6;int result=0;{int a=base,b=a*7;result=b;}return result;}",42)==0);
    return 0;
}
static int qualifiers(void)
{
    CHECK(value("int main(void){const int a=6,b=a*7;return b;}",42)==0);
    CHECK(value("int main(void){int const a=6,b=a*7;return b;}",42)==0);
    const char *bad[]={"int main(void){const int a=1,b=2;b=3;return a;}",
        "int main(void){int const a=1,b=2;a++;return b;}",
        "int main(void){const int a=1,b=2;++b;return a;}"};
    for(size_t i=0;i<sizeof bad/sizeof bad[0];++i){UmcInstruction code[32];size_t count=9;UmcDiagnostic d;
        CHECK(umc_compile(bad[i],strlen(bad[i]),code,32,&count,&d)==UMC_SYNTAX_ERROR && count==0);}
    return 0;
}
static int refusals(void)
{
    const char *bad[]={"int main(void){int a=1,;return a;}","int main(void){int a=1,,b=2;return a;}",
        "int main(void){int a=1,b;return a;}","int main(void){int a=1,b+=2;return a;}",
        "int main(void){int a=1,b=b;return a;}","int main(void){int a=b,b=2;return a;}",
        "int main(void){int a=1,a=2;return a;}","int main(void){int a=1;{int b=2,a=3;}return a;}",
        "int main(void){int a=1,b=2;a=2,b=3;return a;}","int main(void){return (1,2);}",
        "int main(void){int a=(1,2),b=3;return b;}","int main(void){int a=1,int b=2;return b;}",
        "int main(void){int a=1,const b=2;return b;}","int main(void){if(0){int a=1,b=z;}return 0;}"};
    for(size_t i=0;i<sizeof bad/sizeof bad[0];++i){UmcInstruction code[64];size_t count=9;UmcDiagnostic d;
        CHECK(umc_compile(bad[i],strlen(bad[i]),code,64,&count,&d)==UMC_SYNTAX_ERROR && count==0);}
    return 0;
}
static int parity(void)
{
    const char *sources[]={"int main(void){int a=6,b=a*7;return b;}","int main(void){int a=6;int b=a*7;return b;}"};
    UmcInstruction a[32],b[32];size_t an=0,bn=0;UmcDiagnostic d;
    CHECK(umc_compile(sources[0],strlen(sources[0]),a,32,&an,&d)==UMC_OK);
    CHECK(umc_compile(sources[1],strlen(sources[1]),b,32,&bn,&d)==UMC_OK && an==bn && an==8);
    for(size_t i=0;i<an;++i)CHECK(a[i].opcode==b[i].opcode && a[i].operand==b[i].operand);
    UmcResult r={71,73};
    CHECK(umc_execute(a,an,an-1,&r,&d)==UMC_FUEL_EXHAUSTED && r.value==71 && r.instructions_executed==73);
    CHECK(umc_execute(a,an,an,&r,&d)==UMC_OK && r.value==42 && r.instructions_executed==8);
    return 0;
}
static int capacity(void)
{
    char source[2048];size_t used=(size_t)snprintf(source,sizeof source,"int main(void){int ");
    for(size_t i=0;i<UMC_LOCAL_LIMIT;++i){int n=snprintf(source+used,sizeof source-used,"%sv%zu=%zu",i?",":"",i,i);CHECK(n>0 && (size_t)n<sizeof source-used);used+=(size_t)n;}
    const size_t end=used;memcpy(source+used,";return v31;}",14);used+=13;
    UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic d;UmcResult r;
    CHECK(umc_compile(source,used,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK && count==66);
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==31);
    CHECK(umc_compile(source,used,code,65,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0);
    int n=snprintf(source+end,sizeof source-end,",extra=32;return v31;}");CHECK(n>0 && (size_t)n<sizeof source-end);
    CHECK(umc_compile(source,end+(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_LOCAL_CAPACITY && count==0);
    return 0;
}
static int warnings(void)
{
    const char *source="int main(void){int unused=1,used=2,dependent=used+1;return dependent;}";
    UmcSourceReport report;UmcDiagnostic d;
    CHECK(umc_check_source(source,strlen(source),&report,&d)==UMC_OK && report.warning_count==1);
    CHECK(report.warnings[0].source_offset==(size_t)(strstr(source,"unused")-source) && report.warnings[0].source_length==6);
    source="int main(void){int a=1,b=2;return 0;}";
    CHECK(umc_check_source(source,strlen(source),&report,&d)==UMC_OK && report.warning_count==2);
    CHECK(report.warnings[0].source_offset==(size_t)(strstr(source,"a=")-source));
    CHECK(report.warnings[1].source_offset==(size_t)(strstr(source,"b=")-source));
    UmcSourceReport saved=report;source="int main(void){int a=1,b=;return 0;}";
    CHECK(umc_check_source(source,strlen(source),&report,&d)==UMC_SYNTAX_ERROR && memcmp(&saved,&report,sizeof report)==0);
    return 0;
}
static int spans(void)
{
    const char *source="int main(void){int a=6,b=a*7;return b;}";size_t length=strlen(source);
    char exact[80];memcpy(exact,source,length);UmcInstruction code[32];size_t count=0;UmcDiagnostic d;
    CHECK(umc_compile(exact,length,code,32,&count,&d)==UMC_OK);
    for(size_t end=0;end<length;++end)CHECK(umc_compile(exact,end,code,32,&count,&d)!=UMC_OK && count==0);
    /* The original fixture embedded a physical CR/LF inside its C string.
     * Preserve its intended two lines here for review; escaped bytes below keep
     * the host test valid while exercising the guest source line mapping.
     * First line: int main(void){int a=1,
     * Second line:  b=;return a;}
     */
    source="int main(void){int a=1,\r\n b=;return a;}";
    CHECK(umc_compile(source,strlen(source),code,32,&count,&d)==UMC_SYNTAX_ERROR && d.line==2 && d.column==4);
    return 0;
}
static int wire(void)
{
    const char *source="int main(void){int a=6,b=a*7;return b;}";
    UmcInstruction code[32],decoded[32];size_t count=0,bytes=0,restored=0;uint8_t buffer[UMC_BYTECODE_LIMIT];UmcDiagnostic d;UmcResult r;
    CHECK(umc_compile(source,strlen(source),code,32,&count,&d)==UMC_OK);
    CHECK(umc_encode(code,count,buffer,sizeof buffer,&bytes,&d)==UMC_OK && buffer[4]==3);
    CHECK(umc_decode(buffer,bytes,decoded,32,&restored,&d)==UMC_OK && restored==count);
    for(size_t i=0;i<count;++i)CHECK(code[i].opcode==decoded[i].opcode && code[i].operand==decoded[i].operand && code[i].source_offset==decoded[i].source_offset);
    CHECK(umc_execute(decoded,restored,restored,&r,&d)==UMC_OK && r.value==42);
    return 0;
}
static int faults(void)
{
    const char *source="int main(void){int a=1/0,b=2147483647+1;return b;}";
    UmcInstruction code[32];size_t count=0;UmcDiagnostic d;UmcResult r={71,73};
    CHECK(umc_compile(source,strlen(source),code,32,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,32,&r,&d)==UMC_ARITHMETIC_ERROR && d.source_offset==(size_t)(strchr(source,'/')-source));
    CHECK(r.value==71 && r.instructions_executed==73);
    return 0;
}
int main(int argc,char **argv)
{
    CHECK(argc==2 && UMC_DECLARATOR_LIST_PROFILE==1U);
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_DECLARATOR_LISTS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_DECLARATOR_LISTS));
    struct Case{const char *name;int(*run)(void);} cases[]={
        {"values",values},{"scope",scope},{"qualifiers",qualifiers},{"refusals",refusals},{"parity",parity},
        {"capacity",capacity},{"warnings",warnings},{"spans",spans},{"wire",wire},{"faults",faults}};
    for(size_t i=0;i<sizeof cases/sizeof cases[0];++i)if(!strcmp(argv[1],cases[i].name))return cases[i].run();
    return 2;
}
