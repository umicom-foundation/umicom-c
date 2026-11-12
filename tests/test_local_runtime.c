/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_local_runtime.c
 *
 * PURPOSE:
 *   Verify slot isolation, definite initialization, fuel and portable format admission without linking the compiler.
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
/* An independent program uses the highest slot, then overwrites it. */
static const UmcInstruction program[]={
    {UMC_PUSH,6,10U},{UMC_STORE_LOCAL,31,11U},{UMC_LOAD_LOCAL,31,12U},
    {UMC_PUSH,7,13U},{UMC_MULTIPLY,0,14U},{UMC_STORE_LOCAL,31,15U},
    {UMC_LOAD_LOCAL,31,16U},{UMC_RETURN,0,17U}
};
static int execution(void)
{
    UmcDiagnostic d; UmcProgramInfo info; UmcResult r={-99,99U};
    CHECK(umc_inspect(program,8U,&info,&d)==UMC_OK);
    CHECK(info.instruction_count==8U && info.peak_stack==2U && info.constant_count==2U && info.arithmetic_count==1U);
    for(size_t i=0U;i<20U;++i) {
        CHECK(umc_execute(program,8U,8U,&r,&d)==UMC_OK);
        CHECK(r.value==42 && r.instructions_executed==8U);
    }
    /* A prior invocation must never initialize a later invocation's frame. */
    const UmcInstruction uninitialized[]={{UMC_LOAD_LOCAL,31,9U},{UMC_RETURN,0,10U}};
    CHECK(umc_execute(uninitialized,2U,2U,&r,&d)==UMC_BAD_BYTECODE && r.value==42 && r.instructions_executed==8U);
    return 0;
}
static int trace_and_fuel(void)
{
    UmcDiagnostic d; UmcResult r={-99,99U}; UmcTraceStep steps[8]; size_t used=0U;
    CHECK(umc_execute_trace(program,8U,8U,&r,steps,8U,&used,&d)==UMC_OK && used==8U && r.value==42);
    CHECK(steps[1].opcode==UMC_STORE_LOCAL && steps[1].stack_depth==0U && steps[1].top==0);
    CHECK(steps[2].opcode==UMC_LOAD_LOCAL && steps[2].top==6 && steps[2].stack_depth==1U);
    CHECK(steps[5].stack_depth==0U && steps[6].top==42);
    for(size_t fuel=0U;fuel<8U;++fuel) {
        r.value=-99;r.instructions_executed=99U;used=99U;
        CHECK(umc_execute_trace(program,8U,fuel,&r,steps,8U,&used,&d)==UMC_FUEL_EXHAUSTED);
        CHECK(r.value==-99 && r.instructions_executed==99U && used==fuel+1U);
        CHECK(steps[fuel].status==UMC_FUEL_EXHAUSTED && steps[fuel].instruction_index==fuel);
    }
    CHECK(umc_execute_trace(program,8U,8U,&r,steps,7U,&used,&d)==UMC_BUFFER_TOO_SMALL && used==0U && r.value==-99);
    return 0;
}
static int malformed(void)
{
    static const UmcInstruction cases[][4]={
        {{UMC_LOAD_LOCAL,0,7U},{UMC_RETURN,0,8U}},
        {{UMC_PUSH,1,0U},{UMC_STORE_LOCAL,-1,7U},{UMC_PUSH,1,8U},{UMC_RETURN,0,9U}},
        {{UMC_PUSH,1,0U},{UMC_STORE_LOCAL,32,7U},{UMC_PUSH,1,8U},{UMC_RETURN,0,9U}},
        {{UMC_PUSH,1,0U},{UMC_STORE_LOCAL,0,1U},{UMC_LOAD_LOCAL,1,7U},{UMC_RETURN,0,9U}},
        {{UMC_STORE_LOCAL,0,7U},{UMC_PUSH,1,8U},{UMC_RETURN,0,9U}},
        {{UMC_PUSH,1,0U},{UMC_STORE_LOCAL,0,1U},{UMC_LOAD_LOCAL,-1,7U},{UMC_RETURN,0,9U}},
        {{UMC_PUSH,1,0U},{UMC_STORE_LOCAL,0,1U},{UMC_LOAD_LOCAL,32,7U},{UMC_RETURN,0,9U}}
    };
    static const size_t sizes[]={2U,4U,4U,4U,3U,4U,4U};
    for(size_t i=0U;i<sizeof sizes/sizeof sizes[0];++i) {
        UmcResult r={-99,99U}; UmcProgramInfo info={77U,77U,77U,77U,77U};
        UmcTraceStep steps[4]; memset(steps,0x5a,sizeof steps); unsigned char before[sizeof steps];
        memcpy(before,steps,sizeof steps); size_t used=91U; UmcDiagnostic d;
        CHECK(umc_execute_trace(cases[i],sizes[i],0U,&r,steps,4U,&used,&d)==UMC_BAD_BYTECODE);
        CHECK(d.source_offset==7U && used==0U && r.value==-99 && r.instructions_executed==99U);
        CHECK(memcmp(before,steps,sizeof steps)==0);
        CHECK(umc_inspect(cases[i],sizes[i],&info,&d)==UMC_BAD_BYTECODE && info.instruction_count==77U);
    }
    UmcInstruction crowded[UMC_STACK_LIMIT+4U];
    crowded[0]=(UmcInstruction){UMC_PUSH,1,0U}; crowded[1]=(UmcInstruction){UMC_STORE_LOCAL,0,1U};
    for(size_t i=0U;i<UMC_STACK_LIMIT+1U;++i)crowded[i+2U]=(UmcInstruction){UMC_LOAD_LOCAL,0,i+2U};
    crowded[UMC_STACK_LIMIT+3U]=(UmcInstruction){UMC_RETURN,0,99U}; UmcDiagnostic d;
    CHECK(umc_validate(crowded,UMC_STACK_LIMIT+4U,&d)==UMC_STACK_OVERFLOW && d.source_offset==66U);
    return 0;
}
static int wire(void)
{
    /* Fixed bytes were written independently of umc_encode. Slot 31 is legal;
     * records retain nine-byte little-endian layout and stable opcode IDs. */
    static const uint8_t fixed[]={
        'U','M','C','B',3,0,4,0,
        1,42,0,0,0,0,0,0,0,
        15,31,0,0,0,1,0,0,0,
        14,31,0,0,0,2,0,0,0,
        7,0,0,0,0,3,0,0,0
    };
    UmcInstruction decoded[4]; UmcDiagnostic d; UmcResult r; size_t n=0U,bytes=0U;
    uint8_t encoded[sizeof fixed];
    CHECK(umc_decode(fixed,sizeof fixed,decoded,4U,&n,&d)==UMC_OK && n==4U);
    CHECK(umc_execute(decoded,n,n,&r,&d)==UMC_OK && r.value==42);
    CHECK(umc_encode(decoded,n,encoded,sizeof encoded,&bytes,&d)==UMC_OK && bytes==sizeof fixed);
    CHECK(memcmp(encoded,fixed,sizeof fixed)==0);
    for(uint8_t v=1;v<=4;++v) {
        if(v==3)continue;
        memcpy(encoded,fixed,sizeof fixed);encoded[4]=v;n=99U;
        CHECK(umc_decode(encoded,sizeof encoded,decoded,4U,&n,&d)==UMC_BAD_BYTECODE && n==0U);
    }
    memcpy(encoded,fixed,sizeof fixed);encoded[8U+2U*9U+1U]=30U;n=99U;
    CHECK(umc_decode(encoded,sizeof encoded,decoded,4U,&n,&d)==UMC_BAD_BYTECODE && n==0U);
    /* A preceding comparison cannot cause the encoder to stop at format 2. */
    const UmcInstruction mixed[]={
        {UMC_PUSH,1,0U},{UMC_PUSH,2,1U},{UMC_LESS,0,2U},
        {UMC_STORE_LOCAL,0,3U},{UMC_LOAD_LOCAL,0,4U},{UMC_RETURN,0,5U}
    };
    uint8_t larger[UMC_BYTECODE_LIMIT];
    CHECK(umc_encode(mixed,6U,larger,sizeof larger,&bytes,&d)==UMC_OK && larger[4]==3U);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"execution")==0)return execution();
    if(strcmp(argv[1],"trace-fuel")==0)return trace_and_fuel();
    if(strcmp(argv[1],"malformed")==0)return malformed();
    if(strcmp(argv[1],"wire")==0)return wire();
    return 2;
}
