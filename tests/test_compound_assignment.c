/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_compound_assignment.c
 *
 * PURPOSE:
 *   Verify checked local updates, exact lowering, bounded spans, faults and native-C agreement.
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

static int values(void)
{
    static const struct { const char *body; int32_t expected; } cases[] = {
        {"int x=6;x+=7;return x;",13}, {"int x=6;x-=7;return x;",-1},
        {"int x=6;x*=7;return x;",42}, {"int x=42;x/=7;return x;",6},
        {"int x=-7;x/=3;return x;",-2}, {"int x=7;x/=-3;return x;",-2},
        {"int x=-7;x/=-3;return x;",2}, {"int x=7;x-=x;return x;",0},
        {"int x=7;x+=x*2;return x;",21}, {"int x=7;x*=x+1;return x;",56},
        {"int x=2;int y=3;x+=y;y*=x;return x+y;",20},
        {"int x=5;x+=2*3;return x;",11},
        {"int x=5;x*=0?1/0:7;return x;",35},
        {"int x=5;x+=0&&1/0;return x;",5},
        {"int x=5;x-=1||1/0;return x;",4},
        {"int x=5;x/=0?0:5;return x;",1},
        {"int x=5;x/*name*/+=/*value*/0x25;return x;",42},
        {"int x=40;x+=0b10;return x;",42},
        {"int x=(-2147483647-1);x+=2147483647;return x;",-1},
        {"int x=2147483647;x-=2147483647;return x;",0},
        {"int x=6;x+=1;x*=7;x-=7;x/=2;return x;",21}
    };
    for (size_t i=0U; i<sizeof cases/sizeof cases[0]; ++i) {
        char source[256];
        int length=snprintf(source,sizeof source,"int main(void){%s}",cases[i].body);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; UmcResult r; size_t count=0U;
        CHECK(umc_compile(source,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==cases[i].expected);
    }
    return 0;
}
static int faults(void)
{
    static const struct { const char *body; const char *marker; UmcOpcode opcode; } cases[] = {
        {"int x=2147483647;x+=1;return x;","+=",UMC_ADD},
        {"int x=(-2147483647-1);x-=1;return x;","-=",UMC_SUBTRACT},
        {"int x=1073741824;x*=2;return x;","*=",UMC_MULTIPLY},
        {"int x=(-2147483647-1);x/=-1;return x;","/=",UMC_DIVIDE},
        {"int x=42;x/=0;return x;","/=",UMC_DIVIDE},
        {"int x=42;x+=1/0;return x;","/0",UMC_DIVIDE},
        {"int x=0;x*=1/0;return x;","/0",UMC_DIVIDE},
        {"int x=42;x+=1?1/0:7;return x;","/0",UMC_DIVIDE}
    };
    for (size_t i=0U; i<sizeof cases/sizeof cases[0]; ++i) {
        char source[256];
        int length=snprintf(source,sizeof source,"int main(void){%s}",cases[i].body);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcTraceStep rows[UMC_PROGRAM_LIMIT];
        UmcDiagnostic d; UmcResult r={73,91U}; size_t count=0U,used=0U;
        CHECK(umc_compile(source,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute_trace(code,count,count,&r,rows,UMC_PROGRAM_LIMIT,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==73 && r.instructions_executed==91U);
        CHECK(used>0U && rows[used-1U].opcode==cases[i].opcode);
        CHECK(rows[used-1U].status==UMC_ARITHMETIC_ERROR);
        CHECK(d.source_offset==(size_t)(strstr(source,cases[i].marker)-source));
        /* The only completed store is the initializer. The failed arithmetic
         * must never publish a subsequent assignment or a result. */
        size_t stores=0U;
        for(size_t n=0U;n<used;++n) if(rows[n].opcode==UMC_STORE_LOCAL) ++stores;
        CHECK(stores==1U);
    }
    return 0;
}
static int refusals(void)
{
    static const char *const bodies[] = {
        "int x+=1;return x;", "int x-=1;return x;", "int x*=1;return x;", "int x/=1;return x;",
        "x+=1;return 0;", "int x=1;x+=;return x;", "int x=1;x+ =2;return x;",
        "int x=1;x+/**/=2;return x;", "int x=1;x / =2;return x;",
        "int x=1;x+=missing;return x;", "int x=1;return x+=2;",
        "int x=1;int y=x+=2;return y;", "int x=1;int y=2;x+=y+=3;return x;",
/* Remainder assignment is now accepted; retain the previous refusal and
 * exercise a separated operator that remains invalid. */
#if 0
        "int x=1;x+=x=2;return x;", "int x=1;x%=2;return x;",
#endif
        "int x=1;x+=x=2;return x;", "int x=1;x% =2;return x;",
/* A complete bitwise compound update is supported; separated assignment tokens still refuse. */
#if 0
        "int x=1;x&=2;return x;", "int x=1;x++;return x;", "int x=1;++x;return x;",
#endif
/* Standalone ++ is now supported and covered positively; expression-valued and separated updates remain refusals. The former implementation is retained for owner review. */
#if 0
        "int x=1;x& =2;return x;", "int x=1;x++;return x;", "int x=1;++x;return x;",
#endif
        "int x=1;x& =2;return x;", "int x=1;return x++;", "int x=1;return ++x;",
        "int x=1;x===2;return x;", "int x=1;x+=2 return x;"
    };
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i) {
        char source[256]; int length=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; size_t count=99U;
        CHECK(umc_compile(source,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
        CHECK(count==0U && d.source_offset<(size_t)length);
    }
    return 0;
}
static int spans(void)
{
    const char source[]="int main(void){int x=6;x*=7;return x;}";
    char exact[sizeof source-1U]; memcpy(exact,source,sizeof exact);
    UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; size_t count=99U;
    CHECK(umc_compile(exact,sizeof exact,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK && count==8U);
    /* 24 is the independently enumerated '*' byte, 25 its '=' suffix. All
     * truncated prefixes fail without accepting a token beyond the span. */
    for(size_t length=24U;length<=27U;++length) {
        CHECK(umc_compile(exact,length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
        CHECK(count==0U && d.source_offset<=length);
    }
    exact[25]='\0';
    CHECK(umc_compile(exact,sizeof exact,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    const char newline[]="int main(void){\r\nint x=6;\r\nx/=0;\r\nreturn x;}";
    UmcResult r={73,91U};
    CHECK(umc_compile(newline,sizeof newline-1U,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_ARITHMETIC_ERROR);
    umc_locate_diagnostic(newline,sizeof newline-1U,&d);
    CHECK(d.line==3U && d.column==2U);
    return 0;
}
static int bounds(void)
{
    for(size_t updates=63U;updates<=64U;++updates) {
        char source[1024]; size_t length=0U;
        const char start[]="int main(void){int x=0;";
        memcpy(source,start,sizeof start-1U);length=sizeof start-1U;
        for(size_t i=0U;i<updates;++i){memcpy(source+length,"x+=1;",5U);length+=5U;}
        memcpy(source+length,"return x;}",10U);length+=10U;
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; UmcResult r; size_t count=99U;
        UmcStatus status=umc_compile(source,length,code,UMC_PROGRAM_LIMIT,&count,&d);
        if(updates==63U) {
            CHECK(status==UMC_OK && count==UMC_PROGRAM_LIMIT);
            CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==63);
        } else CHECK(status==UMC_PROGRAM_TOO_LARGE && count==0U);
    }
    const char source[]="int main(void){int x=6;x*=7;return x;}";
    UmcInstruction code[8]; UmcDiagnostic d; size_t count=99U;
    CHECK(umc_compile(source,sizeof source-1U,code,7U,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    CHECK(umc_compile(source,sizeof source-1U,code,8U,&count,&d)==UMC_OK && count==8U);
    /* Reuse after a refused compile must not retain symbols from that call. */
    const char unknown[]="int main(void){x+=1;return x;}";
    CHECK(umc_compile(unknown,sizeof unknown-1U,code,8U,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    return 0;
}
static int wire_trace(void)
{
    const char source[]="int main(void){int x=6;x*=7;return x;}";
    /* Numeric opcode/offset bytes are independent of compiler-produced data.
     * Source: literal 6 at21, declaration x at19, update x at23, *= at24,
     * literal7 at26, return x at35 and return keyword at28. */
    static const uint8_t expected[]={
        'U','M','C','B',3,0,8,0,
        1,6,0,0,0,21,0,0,0, 15,0,0,0,0,19,0,0,0,
        14,0,0,0,0,23,0,0,0, 1,7,0,0,0,26,0,0,0,
        4,0,0,0,0,24,0,0,0, 15,0,0,0,0,23,0,0,0,
        14,0,0,0,0,35,0,0,0, 7,0,0,0,0,28,0,0,0
    };
    UmcInstruction code[8],decoded[8]; UmcDiagnostic d; UmcResult r;
    UmcTraceStep rows[8]; uint8_t bytes[sizeof expected]; size_t count=0U,size=0U,used=0U;
    CHECK(umc_compile(source,sizeof source-1U,code,8U,&count,&d)==UMC_OK && count==8U);
    CHECK(umc_encode(code,count,bytes,sizeof bytes,&size,&d)==UMC_OK && size==sizeof expected);
    CHECK(memcmp(bytes,expected,sizeof expected)==0);
    CHECK(umc_decode(expected,sizeof expected,decoded,8U,&count,&d)==UMC_OK);
    for(size_t fuel=0U;fuel<=8U;++fuel) {
        r.value=73;r.instructions_executed=91U;
        UmcStatus status=umc_execute_trace(decoded,count,fuel,&r,rows,8U,&used,&d);
        if(fuel==8U) {
            CHECK(status==UMC_OK && r.value==42 && r.instructions_executed==8U && used==8U);
            CHECK(rows[4].opcode==UMC_MULTIPLY && rows[4].top==42 && rows[4].stack_depth==1U);
            CHECK(rows[5].opcode==UMC_STORE_LOCAL && rows[5].stack_depth==0U);
        } else {
            CHECK(status==UMC_FUEL_EXHAUSTED && used==fuel+1U);
            CHECK(rows[fuel].instruction_index==fuel && rows[fuel].status==UMC_FUEL_EXHAUSTED);
            CHECK(r.value==73 && r.instructions_executed==91U);
        }
    }
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_COMPOUND_ASSIGNMENTS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_COMPOUND_ASSIGNMENTS));
    return 0;
}
/* Native C supplies an independent arithmetic oracle. Operands stay small,
 * division skips zero, and no signed overflow or other undefined C behavior
 * is executed. This covers the shared subset, not arbitrary C programs. */
static int native(void)
{
    const char *const operations[]={"+=","-=","*=","/="};
    for(int left=-9;left<=9;++left) for(int right=-9;right<=9;++right)
        for(size_t op=0U;op<4U;++op) {
            if(op==3U && right==0)continue;
            int expected=left;
            switch(op) {
            case 0U: expected+=right;break;
            case 1U: expected-=right;break;
            case 2U: expected*=right;break;
            default: expected/=right;break;
            }
            char source[128]; int length=snprintf(source,sizeof source,
                "int main(void){int x=%d;x%s%d;return x;}",left,operations[op],right);
            CHECK(length>0 && (size_t)length<sizeof source);
            UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; UmcResult r; size_t count=0U;
            CHECK(umc_compile(source,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
            CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==expected);
        }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"values")==0)return values();
    if(strcmp(argv[1],"faults")==0)return faults();
    if(strcmp(argv[1],"refusals")==0)return refusals();
    if(strcmp(argv[1],"spans")==0)return spans();
    if(strcmp(argv[1],"bounds")==0)return bounds();
    if(strcmp(argv[1],"wire-trace")==0)return wire_trace();
    if(strcmp(argv[1],"native")==0)return native();
    return 2;
}
