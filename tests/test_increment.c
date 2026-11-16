/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_increment.c
 *
 * PURPOSE:
 *   Verify standalone prefix/postfix updates, checked endpoints, lexical spans, unchanged wire records and native-C agreement.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
#include "umicom_c/trace.h"
#include "umicom_c/profile.h"
static UmcStatus compile_body(const char *body,UmcInstruction *code,size_t *count,UmcDiagnostic *d){
    char source[1024];
    int n=snprintf(source,sizeof(source),"int main(void){%s}",body);
    if(n<0||(size_t)n>=sizeof(source))return UMC_INVALID_ARGUMENT;
    return umc_compile(source,(size_t)n,code,UMC_PROGRAM_LIMIT,count,d);
}
static int values(void){
    static const struct{const char *body;int32_t value;} cases[]={
        {"int x=41;++x;return x;",42},{"int x=41;x++;return x;",42},
        {"int x=43;--x;return x;",42},{"int x=43;x--;return x;",42},
        {"int x=-1;++x;return x;",0},{"int x=0;--x;return x;",-1},
        {"int x=40;int y=2;++x;y--;x++;return x+y;",43},
        {"int x=0;if(1){++x;x++;}else{--x;}return x;",2},
        {"int x=0;if(0){++x;}else if(1){--x;x--;}return x;",-2},
        {"int x=2147483647;if(0){++x;}return x;",INT32_MAX},
        {"int x=41;/*before*/++/*between*/x/*after*/;return x;",42},
        {"int x=43;x/*between*/--/*after*/;return x;",42}
    };
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t n=0;
        CHECK(compile_body(cases[i].body,code,&n,&d)==UMC_OK);
        CHECK(umc_execute(code,n,n,&r,&d)==UMC_OK&&r.value==cases[i].value);
    }
    return 0;
}
static int refusals(void){
    const char *cases[]={"++x;return 0;","int x=1;return ++x;","int x=1;return x++;",
        "int x=1;int y=x++;return y;","int x=1;x++++;return x;","int x=1;++x++;return x;",
        "int x=1;+ +x;return x;","int x=1;x+/**/+;return x;","int x=1;++(x);return x;",
        "int x=1;++1;return x;","int x=1;x++=2;return x;","int x=1;--x return x;",
        "int x=1;if(0){++missing;}return x;","int x=1;x+++2;return x;"};
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t n=99;
        CHECK(compile_body(cases[i],code,&n,&d)==UMC_SYNTAX_ERROR&&n==0);
    }
    return 0;
}
static int faults(void){
    const char *sources[]={"int main(void){int x=2147483647;++x;return x;}",
        "int main(void){int x=2147483647;x++;return x;}",
        "int main(void){int x=(-2147483647-1);--x;return x;}",
        "int main(void){int x=(-2147483647-1);x--;return x;}"};
    for(size_t i=0;i<4;++i){
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcTraceStep trace[UMC_PROGRAM_LIMIT];
        UmcDiagnostic d;UmcResult r={73,91};size_t n=0,used=0;
        CHECK(umc_compile(sources[i],strlen(sources[i]),code,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK);
        CHECK(umc_execute_trace(code,n,n,&r,trace,UMC_PROGRAM_LIMIT,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==73&&r.instructions_executed==91);
        CHECK(d.source_offset==(size_t)(strstr(sources[i],i<2?"++":"--")-sources[i]));
        size_t stores=0;for(size_t j=0;j<used;++j)if(trace[j].opcode==UMC_STORE_LOCAL)++stores;
        CHECK(stores==1);
    }
    return 0;
}
static int wire_fuel(void){
    const char source[]="int main(void){int x=41;++x;return x;}";
    /* Literal offsets were enumerated from the independent fixed source:
     * initializer 21, declaration 19, operator 24, return name 35, keyword 28. */
    UmcInstruction code[UMC_PROGRAM_LIMIT],decoded[UMC_PROGRAM_LIMIT];
    UmcDiagnostic d;size_t n=0,written=0,count=0;
    uint8_t bytes[UMC_BYTECODE_LIMIT];
    CHECK(umc_compile(source,sizeof(source)-1U,code,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK&&n==8);
    CHECK(code[2].opcode==UMC_LOAD_LOCAL&&code[3].opcode==UMC_PUSH&&code[3].operand==1);
    CHECK(code[4].opcode==UMC_ADD&&code[5].opcode==UMC_STORE_LOCAL);
    CHECK(code[0].source_offset==21&&code[1].source_offset==19);
    for(size_t i=2;i<6;++i)CHECK(code[i].source_offset==24);
    CHECK(code[6].source_offset==35&&code[7].source_offset==28);
    CHECK(umc_encode(code,n,bytes,sizeof(bytes),&written,&d)==UMC_OK&&bytes[4]==3);
    CHECK(umc_decode(bytes,written,decoded,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK&&count==8);
    for(size_t fuel=0;fuel<=8;++fuel){
        UmcResult r={73,91};
        UmcStatus s=umc_execute(decoded,count,fuel,&r,&d);
        if(fuel<8)CHECK(s==UMC_FUEL_EXHAUSTED&&r.value==73&&r.instructions_executed==91);
        else CHECK(s==UMC_OK&&r.value==42&&r.instructions_executed==8);
    }
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_INCREMENT_STATEMENTS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_INCREMENT_STATEMENTS));
    return 0;
}
static int spans(void){
    const char source[]="int main(void){int x=41;x++;return x;}";
    char exact[sizeof(source)-1U];memcpy(exact,source,sizeof(exact));
    UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t n=0;
    CHECK(umc_compile(exact,sizeof(exact),code,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK);
    const size_t op=(size_t)(strstr(source,"++")-source);
    CHECK(umc_compile(exact,op+1U,code,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR&&n==0);
    CHECK(umc_compile(exact,op+2U,code,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR&&n==0);
    exact[op+1U]='\0';
    CHECK(umc_compile(exact,sizeof(exact),code,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR&&n==0);
    return 0;
}
static int bounds(void){
    for(size_t updates=63;updates<=64;++updates){
        char source[512];size_t length=0;
        const char prefix[]="int main(void){int x=0;";
        memcpy(source,prefix,sizeof(prefix)-1U);length=sizeof(prefix)-1U;
        for(size_t i=0;i<updates;++i){memcpy(source+length,"x++;",4U);length+=4U;}
        memcpy(source+length,"return x;}",10U);length+=10U;
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t n=0;
        UmcStatus s=umc_compile(source,length,code,UMC_PROGRAM_LIMIT,&n,&d);
        if(updates==63)CHECK(s==UMC_OK&&n==256&&umc_execute(code,n,n,&r,&d)==UMC_OK&&r.value==63);
        else CHECK(s==UMC_PROGRAM_TOO_LARGE&&n==0);
    }
    return 0;
}
static int native(void){
    const char *forms[]={"++x","x++","--x","x--"};
    for(int initial=-64;initial<=64;++initial)for(size_t f=0;f<4;++f){
        int expected=initial;
        if(f==0)++expected;else if(f==1)expected++;else if(f==2)--expected;else expected--;
        char body[128];int length=snprintf(body,sizeof(body),"int x=%d;%s;return x;",initial,forms[f]);
        CHECK(length>0&&(size_t)length<sizeof(body));
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t n=0;
        CHECK(compile_body(body,code,&n,&d)==UMC_OK);
        CHECK(umc_execute(code,n,n,&r,&d)==UMC_OK&&r.value==expected);
    }
    return 0;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"values"))return values();
    if(!strcmp(argv[1],"refusals"))return refusals();
    if(!strcmp(argv[1],"faults"))return faults();
    if(!strcmp(argv[1],"wire-fuel"))return wire_fuel();
    if(!strcmp(argv[1],"spans"))return spans();
    if(!strcmp(argv[1],"bounds"))return bounds();
    if(!strcmp(argv[1],"native"))return native();
    return 2;
}
