/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_else_if.c
 *
 * PURPOSE:
 *   Verify ordered chained conditions, shared depth, forward graph identity and native-C agreement.
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
#include "umicom_c/trace.h"
static int values(void)
{
    static const struct { const char *body; int32_t value; } cases[] = {
        {"int x=1;if(1){x=42;}else if(1/0){x=7;}else{x=9;}return x;",42},
        {"int x=1;if(0){x=7;}else if(-2){x=42;}else{x=9;}return x;",42},
        {"int x=1;if(0){x=7;}else if(0){x=9;}else{x=42;}return x;",42},
        {"int x=42;if(0){x=7;}else if(0){x=9;}return x;",42},
        {"int x=1;if(0){}else if(0){}else if(1){x=42;}return x;",42},
        {"int x=6;if(0){}else if(x==6){x*=7;}return x;",42},
        {"int x=6;if(0){}else if(1){if(0){x=0;}else if(1){x*=7;}}return x;",42},
        {"int x=6;if(1){if(0){}else if(0){}else{x*=7;}}else if(1/0){x=0;}return x;",42},
        {"int x=6;if(0){}else if(0&&1/0){x=0;}else if(1||1/0){x*=7;}return x;",42},
        {"int x=6;if(0){}else/* gap */if(0?1/0:1){x*=7;}return x;",42},
        {"int x=42;if(0){}else if(1){}else{}return x;",42},
        {"if(0){}else if(1){}int x=42;return x;",42}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        char text[512];int n=snprintf(text,sizeof text,"int main(void){%s}",cases[i].body);
        CHECK(n>0 && (size_t)n<sizeof text);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=0U;
        CHECK(umc_compile(text,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==cases[i].value);
    }
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_ELSE_IF_CHAINS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_ELSE_IF_CHAINS));
    return 0;
}
static int refusals(void)
{
    static const char *const bodies[]={
        "int x=0;if(0){}else if(1)x=1;return x;",
        "if(0){}else if(){}return 0;",
        "int x=1;if(0){}else if(x=1){}return x;",
/* These formerly unsupported declarations are now positive branch-scope
 * cases in test_branch_locals.c. Preserve the former rejection fixtures. */
#if 0
        "if(0){}else if(1){int x=2;}return 0;",
#endif
        "if(0){}else if(1){return 42;}return 0;",
        "if(0){}else if(1){}else if{}return 0;",
        "int x=1;if(1){x=42;}else if(missing){x=7;}return x;",
        "int x=1;if(1){x=42;}else if(0){x=missing;}return x;",
        "if(0){}else{}else if(1){}return 0;",
        "if(0){}else if(1){}else if(0){}",
        /* Standalone blocks are now valid. Keep the old expectation here
         * for review; test_compound_statements.c executes this exact source. */
#if 0
        "int x=1;if(0){}else if(1){{x=2;}}return x;",
#endif

        /* Empty statements now use the common statement dispatcher. Preserve
         * the old refusal; test_empty_statements.c executes this exact body. */
#if 0
        "int x=1;if(0){}else if(1){;}return x;"
#endif
    };
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i){
        char text[512];int n=snprintf(text,sizeof text,"int main(void){%s}",bodies[i]);
        CHECK(n>0 && (size_t)n<sizeof text);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=99U;
        CHECK(umc_compile(text,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
        CHECK(count==0U && d.source_offset<=(size_t)n && d.line!=0U);
    }
    return 0;
}
static int faults(void)
{
    static const char *const bodies[]={
        "int x=1;if(0){}else if(1/0){x=42;}return x;",
        "int x=1;if(0){}else if(1){x=1/0;}return x;",
        "int x=1;if(0){}else if(0){}else{x=1/0;}return x;"
    };
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i){
        char text[256];int n=snprintf(text,sizeof text,"int main(void){%s}",bodies[i]);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r={73,91U};size_t count=0U;
        CHECK(n>0 && (size_t)n<sizeof text);
        CHECK(umc_compile(text,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==73 && r.instructions_executed==91U);
        CHECK(d.source_offset==(size_t)(strstr(text,"/0")-text));
    }
    return 0;
}
static int depth(void)
{
    for(size_t levels=62U;levels<=64U;++levels)for(size_t extra=0U;extra<=1U;++extra){
        char text[4096];size_t length=0U;
        const char prefix[]="int main(void){int x=42;";
        memcpy(text,prefix,sizeof prefix-1U);length=sizeof prefix-1U;
        for(size_t i=0U;i<levels;++i){
            const char *part=i==0U?"if(":"else if(";
            size_t n=strlen(part);memcpy(text+length,part,n);length+=n;
            if(i+1U==levels && extra)text[length++]='!';
            memcpy(text+length,"0){}",4U);length+=4U;
        }
        memcpy(text+length,"return x;}",10U);length+=10U;
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=99U;
        UmcStatus s=umc_compile(text,length,code,UMC_PROGRAM_LIMIT,&count,&d);
        if(levels+extra<UMC_PARSE_DEPTH_LIMIT){
            CHECK(s==UMC_OK);
            CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==42);
        }else CHECK(s==UMC_DEPTH_LIMIT && count==0U);
    }
    return 0;
}
static const char chain[]="int main(void){int x=1;if(0){x=7;}else if(1){x=42;}else{x=9;}return x;}";
static int wire(void)
{
    /* Expected graph is enumerated independently of the compiler's patches. */
    const UmcOpcode op[]={UMC_PUSH,UMC_STORE_LOCAL,UMC_PUSH,UMC_JUMP_IF_ZERO,UMC_PUSH,
        UMC_STORE_LOCAL,UMC_JUMP,UMC_PUSH,UMC_JUMP_IF_ZERO,UMC_PUSH,UMC_STORE_LOCAL,
        UMC_JUMP,UMC_PUSH,UMC_STORE_LOCAL,UMC_LOAD_LOCAL,UMC_RETURN};
    const int32_t operand[]={1,0,0,7,7,0,14,1,12,42,0,14,9,0,0,0};
    UmcInstruction code[16],decoded[16];UmcDiagnostic d;size_t count=0U,bytes=0U;
    uint8_t encoded[UMC_BYTECODE_HEADER_SIZE+16U*UMC_BYTECODE_RECORD_SIZE];
    CHECK(umc_compile(chain,sizeof chain-1U,code,16U,&count,&d)==UMC_OK && count==16U);
    for(size_t i=0U;i<16U;++i)CHECK(code[i].opcode==op[i] && code[i].operand==operand[i]);
    CHECK(umc_encode(code,count,encoded,sizeof encoded,&bytes,&d)==UMC_OK);
    CHECK(bytes==sizeof encoded && encoded[4]==4U && encoded[6]==16U && encoded[7]==0U);
    CHECK(umc_decode(encoded,bytes,decoded,16U,&count,&d)==UMC_OK && count==16U);
    for(size_t i=0U;i<16U;++i)CHECK(decoded[i].opcode==op[i] && decoded[i].operand==operand[i] && decoded[i].source_offset==code[i].source_offset);
    encoded[4]=3U;
    CHECK(umc_decode(encoded,bytes,decoded,16U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    return 0;
}
static int fuel(void)
{
    static const size_t path[]={0U,1U,2U,3U,7U,8U,9U,10U,11U,14U,15U};
    UmcInstruction code[16];UmcDiagnostic d;UmcTraceStep rows[16];size_t count=0U,used=0U;
    CHECK(umc_compile(chain,sizeof chain-1U,code,16U,&count,&d)==UMC_OK);
    for(size_t budget=0U;budget<=11U;++budget){
        UmcResult r={73,91U};
        UmcStatus s=umc_execute_trace(code,count,budget,&r,rows,16U,&used,&d);
        if(budget==11U)CHECK(s==UMC_OK && r.value==42 && r.instructions_executed==11U && used==11U);
        else {CHECK(s==UMC_FUEL_EXHAUSTED && r.value==73 && r.instructions_executed==91U);
            CHECK(used==budget+1U && rows[budget].status==UMC_FUEL_EXHAUSTED);}
        for(size_t i=0U;i<used;++i)CHECK(rows[i].instruction_index==path[i]);
    }
    return 0;
}
static int native(void)
{
    for(int x=-9;x<=9;++x)for(int y=-9;y<=9;++y){
        int expected=1;
        if(x<y)expected=x+y;
        else if(x==y)expected=x*2;
        else if(x<0)expected=-x;
        else expected=x-y;
        char text[256];int n=snprintf(text,sizeof text,
            "int main(void){int x=%d;int y=%d;int r=1;if(x<y){r=x+y;}else if(x==y){r=x*2;}else if(x<0){r=-x;}else{r=x-y;}return r;}",x,y);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=0U;
        CHECK(n>0 && (size_t)n<sizeof text);
        CHECK(umc_compile(text,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==expected);
    }
    return 0;
}
static int capacity(void)
{
    UmcInstruction code[16];UmcDiagnostic d;size_t count=99U;
    CHECK(umc_compile(chain,sizeof chain-1U,code,15U,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    CHECK(umc_compile(chain,sizeof chain-1U,code,16U,&count,&d)==UMC_OK && count==16U);
    return 0;
}
static int locations(void)
{
    const char source[]="int main(void){\r\nint x=1;\r\nif(0){}else if(1/0){x=42;}\r\nreturn x;}";
    UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=0U;
    CHECK(umc_compile(source,sizeof source-1U,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_ARITHMETIC_ERROR);
    CHECK(d.source_offset==(size_t)(strstr(source,"/0")-source));
    umc_locate_diagnostic(source,sizeof source-1U,&d);
    CHECK(d.line==3U && d.column==17U);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"values"))return values();
    if(!strcmp(argv[1],"refusals"))return refusals();
    if(!strcmp(argv[1],"faults"))return faults();
    if(!strcmp(argv[1],"depth"))return depth();
    if(!strcmp(argv[1],"wire"))return wire();
    if(!strcmp(argv[1],"fuel"))return fuel();
    if(!strcmp(argv[1],"native"))return native();
    if(!strcmp(argv[1],"capacity"))return capacity();
    if(!strcmp(argv[1],"locations"))return locations();
    return 2;
}
