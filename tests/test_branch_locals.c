/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_branch_locals.c
 *
 * PURPOSE:
 *   Verify bounded conditional-block declarations, lexical visibility, slot reuse and unchanged runtime wire contracts.
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
#include <stdarg.h>
static UmcStatus compile_body(const char *body,UmcInstruction *code,size_t *count,UmcDiagnostic *d)
{
    char source[8192];
    int n=snprintf(source,sizeof source,"int main(void){%s}",body);
    if(n<0||(size_t)n>=sizeof source)return UMC_INVALID_ARGUMENT;
    return umc_compile(source,(size_t)n,code,UMC_PROGRAM_LIMIT,count,d);
}
static int values(void)
{
    const struct{const char *body;int32_t expected;} cases[]={
        {"if(1){int x=1;}return 0;",0},
        {"int x=1;if(0){x=2;}else{int y=3;}return x;",1},
        {"int x=1;if(0){int y=2;}return x;",1},
        {"if(0){}else if(1){int x=2;}return 0;",0},
        {"int x=1;if(1){const int y=2;}return x;",1},
        {"int x=1;if(0){int const y=2;}return x;",1},
        {"int r=0;if(1){int x=6;const int y=x+1;r=x*y;}return r;",42},
        {"int r=0;if(0){int x=5;r=x;}else{const int x=42;r=x;}return r;",42},
        {"int r=0;if(1){int x=40;if(x){int y=2;r=x+y;}x++;}return r;",42},
        {"int r=0;if(1){const int x=7;r=x;}int x=40;x+=2;return x;",42},
        {"int r=0;if(0){int x=1/0;r=x;}else if(1){int x=42;r=x;}return r;",42},
        {"int r=0;if(1){int x=40;x++;++x;r=x;}return r;",42},
        {"int r=0;if(1){int x=63;x&=42;r=x;}return r;",42}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i){
        UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic d;UmcResult r;
        CHECK(compile_body(cases[i].body,code,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK&&r.value==cases[i].expected);
    }
    return 0;
}
static int visibility(void)
{
    const char *bad[]={
        "if(1){int x=42;}return x;",
        "int r=0;if(1){int x=42;}r=x;return r;",
        "int r=0;if(1){int x=42;}else{r=x;}return r;",
        "int x=1;if(1){int x=2;}return x;",
        "if(1){int x=1;if(1){int x=2;}}return 0;",
        "if(1){int x=1;int x=2;}return 0;",
        "if(1){int x=x;}return 0;",
        "if(1){int x;}return 0;",
        "int r=0;if(1){if(1){int x=1;}r=x;}return r;",
        "int r=0;if(1){const int x=1;x=2;}return r;",
        "int r=0;if(0){const int x=1;++x;}return r;",
        /* Standalone scopes are now admitted; the exact former refusal is
         * exercised as a success in test_compound_statements.c. */
#if 0
        "{int x=1;}return 0;",
#endif
        "if(1){return 2;}return 0;"
    };
    for(size_t i=0U;i<sizeof bad/sizeof bad[0];++i){
        UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=77U;UmcDiagnostic d;
        CHECK(compile_body(bad[i],code,&count,&d)==UMC_SYNTAX_ERROR&&count==0U);
    }
    return 0;
}
static int wire(void)
{
    const char *source="int r=0;if(1){const int x=42;r=x;}return r;";
    UmcInstruction code[UMC_PROGRAM_LIMIT],decoded[UMC_PROGRAM_LIMIT];size_t count=0,written=0,dc=0;
    uint8_t bytes[UMC_BYTECODE_LIMIT];UmcDiagnostic d;UmcResult r;
    CHECK(compile_body(source,code,&count,&d)==UMC_OK&&count==10U);
    CHECK(umc_encode(code,count,bytes,sizeof bytes,&written,&d)==UMC_OK&&bytes[4]==4U);
    CHECK(umc_decode(bytes,written,decoded,UMC_PROGRAM_LIMIT,&dc,&d)==UMC_OK&&dc==count);
    for(size_t i=0;i<count;++i)CHECK(code[i].opcode==decoded[i].opcode&&code[i].operand==decoded[i].operand);
    for(size_t fuel=0;fuel<=count;++fuel){
        r.value=73;r.instructions_executed=91;
        UmcStatus s=umc_execute(decoded,dc,fuel,&r,&d);
        if(fuel<count)CHECK(s==UMC_FUEL_EXHAUSTED&&r.value==73&&r.instructions_executed==91);
        else CHECK(s==UMC_OK&&r.value==42&&r.instructions_executed==10U);
    }
    return 0;
}
static int append(char *text,size_t capacity,size_t *used,const char *format,...)
{
    va_list arguments;va_start(arguments,format);
    int n=vsnprintf(text+*used,capacity-*used,format,arguments);
    va_end(arguments);
    if(n<0||(size_t)n>=capacity-*used)return 1;
    *used+=(size_t)n;return 0;
}
static int limits(void)
{
    for(size_t names=UMC_LOCAL_LIMIT-1U;names<=UMC_LOCAL_LIMIT;++names){
        char body[8192]="";size_t used=0;
        CHECK(!append(body,sizeof body,&used,"int r=0;if(1){"));
        for(size_t i=0;i<names;++i)CHECK(!append(body,sizeof body,&used,"int x%zu=%zu;",i,i));
        CHECK(!append(body,sizeof body,&used,"r=x0;}return r;"));
        UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=99;UmcDiagnostic d;
        UmcStatus s=compile_body(body,code,&count,&d);
        if(names+1U==UMC_LOCAL_LIMIT)CHECK(s==UMC_OK);
        else CHECK(s==UMC_LOCAL_CAPACITY&&count==0U);
    }
    return 0;
}
static int reuse(void)
{
    char body[8192]="";size_t used=0;
    CHECK(!append(body,sizeof body,&used,"int r=0;"));
    for(size_t block=0;block<3;++block){
        CHECK(!append(body,sizeof body,&used,"if(1){"));
        for(size_t i=0;i<29;++i)CHECK(!append(body,sizeof body,&used,"int x%zu=%zu;",i,i));
        CHECK(!append(body,sizeof body,&used,"r+=x28;}"));
    }
    CHECK(!append(body,sizeof body,&used,"return r;"));
    UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic d;UmcResult r;
    CHECK(compile_body(body,code,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK&&r.value==84);
    for(size_t i=0;i<count;++i)if(code[i].opcode==UMC_STORE_LOCAL||code[i].opcode==UMC_LOAD_LOCAL)
        CHECK(code[i].operand>=0&&code[i].operand<30);
    return 0;
}
static int native(void)
{
    for(int i=-24;i<=24;++i){
        int expected=0;
        if(i>=0){const int positive=i*2;expected=positive+1;}
        else{int magnitude=-i;expected=magnitude*3;}
        char body[256];int n=snprintf(body,sizeof body,
            "int r=0;int input=%d;if(input>=0){const int positive=input*2;r=positive+1;}else{int magnitude=-input;r=magnitude*3;}return r;",i);
        CHECK(n>0&&(size_t)n<sizeof body);
        UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic d;UmcResult r;
        CHECK(compile_body(body,code,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK&&r.value==expected);
    }
    return 0;
}
static int faults(void)
{
    const char source[]="int main(void){int r=0;if(1){const int x=1/0;r=x;}return r;}";
    UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic d;UmcResult r={73,91};
    CHECK(umc_compile(source,sizeof source-1U,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_ARITHMETIC_ERROR);
    CHECK(r.value==73&&r.instructions_executed==91&&d.source_offset==(size_t)(strchr(source,'/')-source));
    return 0;
}
static int spans(void)
{
    const char source[]="int main(void){\r\nif(1){int hidden=1;}\r\nreturn hidden;}";
    UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic d;
    CHECK(umc_compile(source,sizeof source-1U,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
    CHECK(d.line==3U&&d.column==8U&&count==0U);
    const char valid[]="int main(void){int r=0;if(1){int x=42;r=x;}return r;}";
    char exact[sizeof valid-1U];memcpy(exact,valid,sizeof exact);
    CHECK(umc_compile(exact,sizeof exact,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    for(size_t length=0;length<sizeof exact;++length)
        CHECK(umc_compile(exact,length,code,UMC_PROGRAM_LIMIT,&count,&d)!=UMC_OK&&count==0U);
    return 0;
}
static int depth(void)
{
    char body[8192]="";size_t used=0;
    for(size_t i=0;i<UMC_PARSE_DEPTH_LIMIT+1U;++i)CHECK(!append(body,sizeof body,&used,"if(1){"));
    for(size_t i=0;i<UMC_PARSE_DEPTH_LIMIT+1U;++i)CHECK(!append(body,sizeof body,&used,"}"));
    CHECK(!append(body,sizeof body,&used,"return 0;"));
    UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=91;UmcDiagnostic d;
    CHECK(compile_body(body,code,&count,&d)==UMC_DEPTH_LIMIT&&count==0U);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    CHECK(UMC_BRANCH_LOCAL_PROFILE==1U);
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_BRANCH_LOCALS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_BRANCH_LOCALS));
    if(!strcmp(argv[1],"values"))return values();
    if(!strcmp(argv[1],"visibility"))return visibility();
    if(!strcmp(argv[1],"wire"))return wire();
    if(!strcmp(argv[1],"limits"))return limits();
    if(!strcmp(argv[1],"reuse"))return reuse();
    if(!strcmp(argv[1],"native"))return native();
    if(!strcmp(argv[1],"faults"))return faults();
    if(!strcmp(argv[1],"spans"))return spans();
    if(!strcmp(argv[1],"depth"))return depth();
    return 2;
}
