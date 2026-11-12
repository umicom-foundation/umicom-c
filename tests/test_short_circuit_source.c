/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_short_circuit_source.c
 *
 * PURPOSE:
 *   Verify left-to-right logical evaluation, precedence, span boundaries and branch source identity.
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
    static const struct { const char *expression; int32_t value; } cases[] = {
        {"0 && (1/0)",0},{"9 || (1/0)",1},{"0 || -7",1},{"-2 && 3",1},
        {"1 && 0",0},{"0 || 0",0},{"1 || 0 && 0",1},{"(1 || 0) && 0",0},
        {"2 == 2 && 3 < 4",1},{"0 && 1 || 2 && -3",1},
        {"0 && (2147483647+1)",0},{"1 || (-(-2147483647-1))",1},
        {"0 && 1 && (1/0)",0},{"1 || 0 || (1/0)",1},
        {"!(0 || 3) + (2 && 9)",1},{"0 || /* rhs */ 0 || 8",1}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        char source[256]; int n=snprintf(source,sizeof source,"int main(void){return %s;}",cases[i].expression);
        CHECK(n>0 && (size_t)n<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT], decoded[UMC_PROGRAM_LIMIT];
        UmcDiagnostic d; UmcResult r; size_t count=0U, bytes=0U, restored=0U;
        uint8_t wire[UMC_BYTECODE_LIMIT];
        CHECK(umc_compile(source,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==cases[i].value);
        CHECK(r.instructions_executed<=count);
        CHECK(umc_encode(code,count,wire,sizeof wire,&bytes,&d)==UMC_OK && wire[4]==4U);
        CHECK(umc_decode(wire,bytes,decoded,UMC_PROGRAM_LIMIT,&restored,&d)==UMC_OK);
        CHECK(umc_execute(decoded,restored,restored,&r,&d)==UMC_OK && r.value==cases[i].value);
        for(size_t j=0U;j<count;++j) if(code[j].opcode==UMC_JUMP || code[j].opcode==UMC_JUMP_IF_ZERO) {
            CHECK(code[j].operand>(int32_t)j && (size_t)code[j].operand<count);
            CHECK(source[code[j].source_offset]=='&' || source[code[j].source_offset]=='|');
        }
    }
    return 0;
}
static int faults(void)
{
    static const char *const cases[]={"1 && (1/0)","0 || (1/0)","(1/0) && 0","(1/0) || 1"};
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        char source[128]; int n=snprintf(source,sizeof source,"int main(void){return %s;}",cases[i]);
        CHECK(n>0 && (size_t)n<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; UmcResult r={19,23U}; size_t count=0U,used=0U;
        UmcTraceStep trace[UMC_PROGRAM_LIMIT];
        CHECK(umc_compile(source,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute_trace(code,count,count,&r,trace,UMC_PROGRAM_LIMIT,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==19 && r.instructions_executed==23U && used>0U);
        CHECK(trace[used-1U].opcode==UMC_DIVIDE && d.source_offset==(size_t)(strchr(source,'/')-source));
    }
    return 0;
}
static int refusals(void)
{
    static const char *const invalid[]={"1 & 2","1 | 2","1 & & 2","1 | | 2","1 &&","|| 1","1 &&& 2","1 ||| 2"};
    UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; size_t count=99U;
    for(size_t i=0U;i<sizeof invalid/sizeof invalid[0];++i) {
        char source[128]; int n=snprintf(source,sizeof source,"int main(void){return %s;}",invalid[i]);
        CHECK(n>0 && (size_t)n<sizeof source);
        CHECK(umc_compile(source,(size_t)n,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    }
    const char span[]="int main(void){return 1 && 2;}";
    size_t first=(size_t)(strchr(span,'&')-span);
    /* A borrowed span ending after the first byte may not consume the second. */
    CHECK(umc_compile(span,first+1U,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    CHECK(d.source_offset==first);
    CHECK(umc_compile(span,sizeof span-1U,code,3U,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"values")==0)return values();
    if(strcmp(argv[1],"faults")==0)return faults();
    if(strcmp(argv[1],"refusals")==0)return refusals();
    return 2;
}
