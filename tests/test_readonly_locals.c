/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_readonly_locals.c
 *
 * PURPOSE:
 *   Verify source-only const admission, write rejection, initialization lifetime and ordinary bytecode compatibility.
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
#include <limits.h>

static UmcStatus compile_body(const char *body, UmcInstruction *code, size_t *count, UmcDiagnostic *d)
{
    char source[4096];
    const int n = snprintf(source, sizeof(source), "int main(void){%s}", body);
    if (n < 0 || (size_t)n >= sizeof(source)) return UMC_INVALID_ARGUMENT;
    return umc_compile(source, (size_t)n, code, UMC_PROGRAM_LIMIT, count, d);
}
static int values(void)
{
    const struct { const char *body; int32_t value; } cases[] = {
        {"const int x=42;return x;",42},
        {"const int x=1,y=2;return x;",1},
        {"int const x=42;return x;",42},
        {"int a=6;const int b=a*7;a=0;return b;",42},
        {"const int a=6;int const b=a+1;int x=0;if(b==7){x=a*b;}return x;",42},
        {"const /* first */ int /* second */ x=0b10'1010;return x;",42},
        {"int /* first */ const /* second */ x=0x2a;return x;",42},
        {"int constant=40;int constancy=2;return constant+constancy;",42},
        {"const int x=-2147483647-1;return x;",INT32_MIN},
        {"const int x=2147483647;return x;",INT32_MAX}
    };
    for (size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=0;UmcDiagnostic d;UmcResult r;
        CHECK(compile_body(cases[i].body,code,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==cases[i].value);
    }
    return 0;
}
static int writes(void)
{
    const char *updates[]={"x=1;","x+=1;","x-=1;","x*=2;","x/=2;","x%=2;",
        "x&=1;","x^=1;","x|=1;","++x;","--x;","x++;","x--;"};
    for(size_t qualifier=0;qualifier<2;++qualifier) for(size_t branch=0;branch<3;++branch)
    for(size_t i=0;i<sizeof(updates)/sizeof(updates[0]);++i) {
        char body[256];const char *q=qualifier==0?"const int":"int const";
        int n=snprintf(body,sizeof(body),"%s x=7;%s%s%sreturn x;",q,
            branch==0?"":branch==1?"if(0){":"if(1){}else{",updates[i],branch==0?"":"}");
        CHECK(n>0 && (size_t)n<sizeof(body));
        UmcInstruction code[UMC_PROGRAM_LIMIT];size_t count=91;UmcDiagnostic d;
        CHECK(compile_body(body,code,&count,&d)==UMC_SYNTAX_ERROR && count==0);
        CHECK(d.message!=NULL && strstr(d.message,"read-only")!=NULL);
    }
    return 0;
}
static int syntax(void)
{
    const char *bad[]={"const x=1;return x;","const const int x=1;return x;",
        "int const const x=1;return x;","const int const x=1;return x;",
        "const int x;return x;","int const x+=1;return x;",
        "const int x=x;return x;","const int x=y;int y=2;return x;",
        "const int x=1;int x=2;return x;","int x=1;const int x=2;return x;",
/* These formerly unsupported declarations are now positive branch-scope
 * cases in test_branch_locals.c. Preserve the former rejection fixtures. */
#if 0
        "int x=1;if(1){const int y=2;}return x;","int x=1;if(0){int const y=2;}return x;",
#endif
/* The const prefix now applies to every initialized declarator. Preserve the former syntax refusal; values executes its exact accepted body. The former implementation is retained for owner review. */
#if 0
        "int const=1;return const;","const int x=1,y=2;return x;",
#endif
        "int const=1;return const;",
        "const int x=1;return const x;"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i){
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=7;
        CHECK(compile_body(bad[i],code,&count,&d)==UMC_SYNTAX_ERROR && count==0);
    }
    return 0;
}
static int faults(void)
{
    const char source[]="int main(void){const int z=0;int const x=1/z;return x;}";
    UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=0;
    CHECK(umc_compile(source,sizeof(source)-1,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    UmcResult r={73,91};
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_ARITHMETIC_ERROR);
    CHECK(r.value==73 && r.instructions_executed==91);
    CHECK(d.source_offset==(size_t)(strchr(source,'/')-source));
    return 0;
}
static int wire(void)
{
    const char *sources[]={"int main(void){const int x=42;return x;}","int main(void){int x=42;return x;}"};
    UmcInstruction a[UMC_PROGRAM_LIMIT],b[UMC_PROGRAM_LIMIT],decoded[UMC_PROGRAM_LIMIT];
    UmcDiagnostic d;size_t na=0,nb=0,nd=0,written=0;uint8_t bytes[UMC_BYTECODE_LIMIT];
    CHECK(umc_compile(sources[0],strlen(sources[0]),a,UMC_PROGRAM_LIMIT,&na,&d)==UMC_OK);
    CHECK(umc_compile(sources[1],strlen(sources[1]),b,UMC_PROGRAM_LIMIT,&nb,&d)==UMC_OK);
    CHECK(na==4 && nb==na);
    for(size_t i=0;i<na;++i) CHECK(a[i].opcode==b[i].opcode && a[i].operand==b[i].operand);
    CHECK(umc_encode(a,na,bytes,sizeof(bytes),&written,&d)==UMC_OK && bytes[4]==3);
    CHECK(umc_decode(bytes,written,decoded,UMC_PROGRAM_LIMIT,&nd,&d)==UMC_OK && nd==na);
    for(size_t fuel=0;fuel<=na;++fuel){
        UmcResult r={73,91};UmcStatus s=umc_execute(decoded,nd,fuel,&r,&d);
        if(fuel<na) CHECK(s==UMC_FUEL_EXHAUSTED && r.value==73 && r.instructions_executed==91);
        else CHECK(s==UMC_OK && r.value==42 && r.instructions_executed==na);
    }
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_READONLY_LOCALS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_READONLY_LOCALS));
    return 0;
}
static int spans(void)
{
    const char source[]="int main(void){\r\nconst int x=1;\r\nx=2;\r\nreturn x;}";
    UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=0;
    CHECK(umc_compile(source,sizeof(source)-1,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
    CHECK(d.line==3 && d.column==1 && d.source_offset==(size_t)(strstr(source,"x=2")-source));
    const char valid[]="int main(void){int const x=42;return x;}";
    char exact[sizeof(valid)-1];memcpy(exact,valid,sizeof(exact));
    CHECK(umc_compile(exact,sizeof(exact),code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    for(size_t length=0;length<sizeof(exact);++length){
        CHECK(umc_compile(exact,length,code,UMC_PROGRAM_LIMIT,&count,&d)!=UMC_OK && count==0);
    }
    exact[20]='\0';
    CHECK(umc_compile(exact,sizeof(exact),code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
    return 0;
}
static int limits(void)
{
    for(size_t locals=UMC_LOCAL_LIMIT;locals<=UMC_LOCAL_LIMIT+1U;++locals){
        char source[4096];size_t used=0;
        for(size_t i=0;i<locals;++i){
            int n=snprintf(source+used,sizeof(source)-used,"const int x%zu=%zu;",i,i);
            CHECK(n>0 && (size_t)n<sizeof(source)-used);used+=(size_t)n;
        }
        memcpy(source+used,"return x0;",11);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=7;
        UmcStatus s=compile_body(source,code,&count,&d);
        if(locals==UMC_LOCAL_LIMIT) CHECK(s==UMC_OK && count==2U*locals+2U);
        else CHECK(s==UMC_LOCAL_CAPACITY && count==0);
    }
    return 0;
}
static int native(void)
{
    for(int i=-128;i<=128;++i){
        int original=i;const int captured=original*3+7;original=0;
        (void)original;
        char body[160];int n=snprintf(body,sizeof(body),"int a=%d;const int b=a*3+7;a=0;return b;",i);
        CHECK(n>0 && (size_t)n<sizeof(body));
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=0;
        CHECK(compile_body(body,code,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==captured);
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"values"))return values();
    if(!strcmp(argv[1],"writes"))return writes();
    if(!strcmp(argv[1],"syntax"))return syntax();
    if(!strcmp(argv[1],"faults"))return faults();
    if(!strcmp(argv[1],"wire"))return wire();
    if(!strcmp(argv[1],"spans"))return spans();
    if(!strcmp(argv[1],"limits"))return limits();
    if(!strcmp(argv[1],"native"))return native();
    return 2;
}
