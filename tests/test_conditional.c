/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_conditional.c
 *
 * PURPOSE:
 *   Verify conditional values, associativity, selected faults, recursive bounds, trace identity and canonical branch lowering.
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

static int values(void)
{
    static const struct {const char *expression;int32_t value;} cases[]={
        {"1 ? 7 : 42",7},{"0 ? 7 : 42",42},{"-3 ? -9 : 0",-9},
        {"0 ? 1/0 : 42",42},{"1 ? 42 : 1/0",42},
        {"0 ? 1 : 1 ? 7 : 8",7},{"1 ? 0 : 1 ? 7 : 8",0},
        {"1 ? 0 ? 7 : 8 : 9",8},{"0 ? 1 ? 7 : 8 : 9",9},
        {"1 || 0 ? 7 : 9",7},{"0 && 1 ? 7 : 9",9},
        {"0 ? 7 : 1 || 0",1},{"1 ? (0 || 3) : 4",1},
        {"(0 ? 7 : 8) + 3",11},{"1 ? (-2147483647-1) : 0",INT32_MIN},
        {"0 ? (2147483647+1) : 6",6},{"1 /* c */ ? 2 /* t */ : 3",2}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i){
        char source[256];int n=snprintf(source,sizeof source,"int main(void){return %s;}",cases[i].expression);
        CHECK(n>0 && (size_t)n<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=0U;
        CHECK(umc_compile(source,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==cases[i].value);
    }
    const char local[]="int main(void){int x=0;int y=x?1/x:42; x=y>40?y-1:0;return x;}";
    UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=0U;
    CHECK(umc_compile(local,sizeof local-1U,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==41);
    return 0;
}
static int faults(void)
{
    static const char *const expressions[]={"1 ? 1/0 : 42","0 ? 42 : 1/0","(1/0) ? 7 : 9"};
    for(size_t i=0U;i<sizeof expressions/sizeof expressions[0];++i){
        char source[128];int n=snprintf(source,sizeof source,"int main(void){return %s;}",expressions[i]);
        CHECK(n>0 && (size_t)n<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcTraceStep rows[UMC_PROGRAM_LIMIT];
        UmcDiagnostic d;UmcResult r={73,91U};size_t count=0U,used=0U;
        CHECK(umc_compile(source,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute_trace(code,count,count,&r,rows,UMC_PROGRAM_LIMIT,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(used>0U && rows[used-1U].opcode==UMC_DIVIDE);
        CHECK(d.source_offset==(size_t)(strchr(source,'/')-source));
        CHECK(r.value==73 && r.instructions_executed==91U);
    }
    return 0;
}
static int refusals(void)
{
    static const char *const invalid[]={"1 ? : 2","1 ? 2","1 ? 2 :","? 1 : 2","1 : 2", "1 ? 2 : 3 : 4", "1 ? missing : 42","1 ? 42 : missing","1 ? 2,3 : 4","1 ? 2 : (x=3)"};
    for(size_t i=0U;i<sizeof invalid/sizeof invalid[0];++i){
        char source[128];int n=snprintf(source,sizeof source,"int main(void){return %s;}",invalid[i]);
        CHECK(n>0 && (size_t)n<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=99U;
        CHECK(umc_compile(source,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    }
    return 0;
}
static int bounds(void)
{
    /* Nest true arms first so parser depth is reached before capacity. Both
     * directions of right-associative recursion must obey the same limit. */
    for(size_t false_arm=0U;false_arm<2U;++false_arm)for(size_t nesting=63U;nesting<=64U;++nesting){
        char source[2048];size_t n=(size_t)snprintf(source,sizeof source,"int main(void){return ");
        for(size_t i=0U;i<nesting;++i){const char *part=false_arm?"0?0:":"1?";size_t len=strlen(part);memcpy(source+n,part,len);n+=len;}
        source[n++]='7';
        if(!false_arm)for(size_t i=0U;i<nesting;++i){source[n++]=':';source[n++]='0';}
        source[n++]=';';source[n++]='}';
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=99U;
        UmcStatus status=umc_compile(source,n,code,UMC_PROGRAM_LIMIT,&count,&d);
        if(nesting==63U){
            CHECK(status==UMC_OK && count==254U);
            CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==7);
        }else CHECK(status==UMC_DEPTH_LIMIT && count==0U);
    }
    const char source[]="int main(void){return 0?7:42;}";
    char exact[sizeof source-1U];memcpy(exact,source,sizeof exact);
    UmcInstruction code[8];UmcDiagnostic d;size_t count=99U;
    CHECK(umc_compile(exact,sizeof exact,code,8U,&count,&d)==UMC_OK && count==6U);
    CHECK(umc_compile(source,(size_t)(strchr(source,':')-source)+1U,code,8U,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    CHECK(umc_compile(source,sizeof source-1U,code,5U,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    return 0;
}
static int wire_trace(void)
{
    /* The initial manual vector omitted the space after return. Preserve it
     * for review; the active vector uses independently enumerated source bytes. */
#if 0
    /* Enumerated bytes for the exact source below: condition at 21, question
     * at 22, true value at 23, false value at 25, return keyword at 15. */
    static const uint8_t expected[]={
        'U','M','C','B',4,0,6,0,
        1,0,0,0,0,21,0,0,0,
        17,4,0,0,0,22,0,0,0,
        1,7,0,0,0,23,0,0,0,
        16,5,0,0,0,22,0,0,0,
        1,42,0,0,0,25,0,0,0,
        7,0,0,0,0,15,0,0,0
    };
#endif
    /* Enumerated bytes for the exact source below: condition at 22, question
     * at 23, true value at 24, false value at 26, return keyword at 15. */
    static const uint8_t expected[]={
        'U','M','C','B',4,0,6,0,
        1,0,0,0,0,22,0,0,0,
        17,4,0,0,0,23,0,0,0,
        1,7,0,0,0,24,0,0,0,
        16,5,0,0,0,23,0,0,0,
        1,42,0,0,0,26,0,0,0,
        7,0,0,0,0,15,0,0,0
    };
    const char source[]="int main(void){return 0?7:42;}";
    UmcInstruction code[6],decoded[6];UmcTraceStep rows[6];UmcDiagnostic d;UmcResult r;
    uint8_t bytes[sizeof expected];size_t count=0U,size=0U,used=0U;
    CHECK(umc_compile(source,sizeof source-1U,code,6U,&count,&d)==UMC_OK && count==6U);
    CHECK(umc_encode(code,count,bytes,sizeof bytes,&size,&d)==UMC_OK && size==sizeof expected);
    CHECK(memcmp(bytes,expected,sizeof expected)==0);
    CHECK(umc_decode(expected,sizeof expected,decoded,6U,&count,&d)==UMC_OK);
    CHECK(umc_execute_trace(decoded,count,4U,&r,rows,6U,&used,&d)==UMC_OK);
    CHECK(r.value==42 && used==4U && r.instructions_executed==4U);
    CHECK(rows[2].instruction_index==4U && rows[3].instruction_index==5U);
    decoded[0].operand=1;
    CHECK(umc_execute_trace(decoded,count,5U,&r,rows,6U,&used,&d)==UMC_OK && r.value==7 && used==5U);
    CHECK(rows[3].opcode==UMC_JUMP && rows[4].instruction_index==5U);
    r.value=73;r.instructions_executed=91U;
    CHECK(umc_execute_trace(decoded,count,3U,&r,rows,6U,&used,&d)==UMC_FUEL_EXHAUSTED);
    /* The old assertion used the same off-by-one manual offset as the vector. */
#if 0
    CHECK(used==4U && rows[3].opcode==UMC_JUMP && d.source_offset==22U);
#endif
    CHECK(used==4U && rows[3].opcode==UMC_JUMP && d.source_offset==(size_t)(strchr(source,'?')-source));
    CHECK(r.value==73 && r.instructions_executed==91U);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"values")==0)return values();
    if(strcmp(argv[1],"faults")==0)return faults();
    if(strcmp(argv[1],"refusals")==0)return refusals();
    if(strcmp(argv[1],"bounds")==0)return bounds();
    if(strcmp(argv[1],"wire-trace")==0)return wire_trace();
    return 2;
}
