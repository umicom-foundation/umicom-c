/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_forward_branches.c
 *
 * PURPOSE:
 *   Admit bounded forward control flow and verify path joins, private locals, execution fuel and canonical wire bytes without a compiler.
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
/* Both structural arms initialize the same slot before joining. Runtime
 * admission must reason about both arms even when the condition is constant. */
static const UmcInstruction diamond[]={
    {UMC_PUSH,1,10U},{UMC_JUMP_IF_ZERO,5,11U},
    {UMC_PUSH,41,12U},{UMC_STORE_LOCAL,31,13U},{UMC_JUMP,7,14U},
    {UMC_PUSH,42,15U},{UMC_STORE_LOCAL,31,16U},
    {UMC_LOAD_LOCAL,31,17U},{UMC_RETURN,0,18U}
};
static int execution(void)
{
    UmcInstruction code[9];memcpy(code,diamond,sizeof code);
    UmcDiagnostic d;UmcProgramInfo info;UmcResult r;
    CHECK(umc_inspect(code,9U,&info,&d)==UMC_OK);
    CHECK(info.instruction_count==9U && info.peak_stack==1U && info.constant_count==3U && info.arithmetic_count==0U);
    CHECK(umc_execute(code,9U,7U,&r,&d)==UMC_OK && r.value==41 && r.instructions_executed==7U);
    code[0].operand=0;
    CHECK(umc_execute(code,9U,6U,&r,&d)==UMC_OK && r.value==42 && r.instructions_executed==6U);
    code[0].operand=-1;
    CHECK(umc_execute(code,9U,7U,&r,&d)==UMC_OK && r.value==41);
    const UmcInstruction next[]={{UMC_JUMP,1,0U},{UMC_PUSH,42,1U},{UMC_RETURN,0,2U}};
    CHECK(umc_execute(next,3U,3U,&r,&d)==UMC_OK && r.value==42 && r.instructions_executed==3U);
    return 0;
}
static int trace_fuel(void)
{
    static const size_t paths[2][7]={{0U,1U,5U,6U,7U,8U,0U},{0U,1U,2U,3U,4U,7U,8U}};
    UmcInstruction code[9];memcpy(code,diamond,sizeof code);
    UmcDiagnostic d;UmcTraceStep steps[9];size_t used=0U;UmcResult r;
    for(size_t truth=0U;truth<2U;++truth) {
        code[0].operand=(int32_t)truth;size_t required=6U+truth;
        CHECK(umc_execute_trace(code,9U,required,&r,steps,9U,&used,&d)==UMC_OK);
        CHECK(used==required && r.instructions_executed==required);
        for(size_t row=0U;row<used;++row)CHECK(steps[row].instruction_index==paths[truth][row]);
        CHECK(steps[1].stack_depth==0U && steps[1].top==0);
        for(size_t fuel=0U;fuel<required;++fuel) {
            r.value=-99;r.instructions_executed=99U;
            CHECK(umc_execute_trace(code,9U,fuel,&r,steps,9U,&used,&d)==UMC_FUEL_EXHAUSTED);
            CHECK(used==fuel+1U && r.value==-99 && r.instructions_executed==99U);
            CHECK(steps[fuel].instruction_index==paths[truth][fuel] && steps[fuel].status==UMC_FUEL_EXHAUSTED);
            CHECK(d.source_offset==code[paths[truth][fuel]].source_offset);
        }
        CHECK(umc_execute_trace(code,9U,required,&r,steps,8U,&used,&d)==UMC_BUFFER_TOO_SMALL && used==0U);
    }
    return 0;
}
static int refuses(const UmcInstruction *code,size_t count)
{
    UmcDiagnostic d;UmcResult r={73,91U};UmcProgramInfo info={71U,72U,73U,74U,75U};
    UmcTraceStep steps[UMC_PROGRAM_LIMIT];memset(steps,0x5a,sizeof steps);
    unsigned char before[sizeof steps];memcpy(before,steps,sizeof before);size_t used=99U;
    CHECK(umc_validate(code,count,&d)==UMC_BAD_BYTECODE);
    CHECK(umc_inspect(code,count,&info,&d)==UMC_BAD_BYTECODE && info.instruction_count==71U && info.bytecode_size==75U);
    CHECK(umc_execute_trace(code,count,count,&r,steps,UMC_PROGRAM_LIMIT,&used,&d)==UMC_BAD_BYTECODE);
    CHECK(r.value==73 && r.instructions_executed==91U && used==0U && memcmp(before,steps,sizeof before)==0);
    return 0;
}
static int malformed(void)
{
    static const int32_t targets[]={-1,0,1,9,INT32_MAX};
    UmcInstruction code[9];
    for(size_t i=0U;i<sizeof targets/sizeof targets[0];++i) {
        memcpy(code,diamond,sizeof code);code[1].operand=targets[i];CHECK(refuses(code,9U)==0);
    }
    memcpy(code,diamond,sizeof code);code[3].opcode=UMC_NEGATE;code[3].operand=0;
    CHECK(refuses(code,9U)==0); /* One incoming path has an extra stack value. */
    memcpy(code,diamond,sizeof code);code[3].operand=30;
    CHECK(refuses(code,9U)==0); /* Slot 31 is initialized only on the false arm. */
    memcpy(code,diamond,sizeof code);code[6].operand=30;
    CHECK(refuses(code,9U)==0); /* Intersection must also account for this arm. */
    memcpy(code,diamond,sizeof code);code[4].operand=3;
    CHECK(refuses(code,9U)==0); /* Back edges cannot turn fuel into loop support. */
    const UmcInstruction hidden[]={{UMC_JUMP,2,0U},{UMC_PUSH,1,1U},{UMC_PUSH,2,2U},{UMC_RETURN,0,3U}};
    CHECK(refuses(hidden,4U)==0);
    const UmcInstruction empty[]={{UMC_JUMP_IF_ZERO,1,0U},{UMC_RETURN,0,1U}};
    CHECK(refuses(empty,2U)==0);
    return 0;
}
static int wire(void)
{
    /* Independent little-endian fixture: false selects record 4 and returns 42. */
    static const uint8_t bytes[]={
        'U','M','C','B',4,0,6,0,
        1,0,0,0,0,0,0,0,0,
        17,4,0,0,0,1,0,0,0,
        1,7,0,0,0,2,0,0,0,
        16,5,0,0,0,3,0,0,0,
        1,42,0,0,0,4,0,0,0,
        7,0,0,0,0,5,0,0,0
    };
    UmcInstruction code[6];uint8_t encoded[sizeof bytes],bad[sizeof bytes];
    UmcDiagnostic d;UmcResult r;size_t count=0U,size=0U;
    CHECK(umc_decode(bytes,sizeof bytes,code,6U,&count,&d)==UMC_OK && count==6U);
    CHECK(umc_execute(code,count,4U,&r,&d)==UMC_OK && r.value==42 && r.instructions_executed==4U);
    CHECK(umc_encode(code,count,encoded,sizeof encoded,&size,&d)==UMC_OK && size==sizeof bytes);
    CHECK(memcmp(encoded,bytes,size)==0);
    for(uint8_t version=1U;version<=5U;++version) {
        if(version==4U)continue;
        memcpy(bad,bytes,sizeof bad);bad[4]=version;count=99U;
        CHECK(umc_decode(bad,sizeof bad,code,6U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    }
    memcpy(bad,bytes,sizeof bad);bad[18]=6U; /* JZ target is past the final record. */
    CHECK(umc_decode(bad,sizeof bad,code,6U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"execution")==0)return execution();
    if(strcmp(argv[1],"trace-fuel")==0)return trace_fuel();
    if(strcmp(argv[1],"malformed")==0)return malformed();
    if(strcmp(argv[1],"wire")==0)return wire();
    return 2;
}
