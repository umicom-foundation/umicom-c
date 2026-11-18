/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_execution_bounds.c
 *
 * PURPOSE:
 *   Verify structural path costs independently of arithmetic execution, source feasibility and wire storage.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
#include "umicom_c/execution_bounds.h"
#include <limits.h>

static const UmcInstruction diamond[] = {
    {UMC_PUSH,1,0},{UMC_JUMP_IF_ZERO,6,1},
    {UMC_PUSH,40,2},{UMC_PUSH,2,3},{UMC_ADD,0,4},{UMC_JUMP,7,5},
    {UMC_PUSH,7,6},{UMC_RETURN,0,7}
};
static int straight(void)
{
    const UmcInstruction program[]={{UMC_PUSH,42,0},{UMC_RETURN,0,1}};
    UmcExecutionBounds bounds={91,92};UmcDiagnostic diagnostic;
    CHECK(umc_execution_bounds(program,2,&bounds,&diagnostic)==UMC_OK);
    CHECK(bounds.minimum_instructions==2 && bounds.maximum_instructions==2);
    CHECK(diagnostic.status==UMC_OK);
    return 0;
}
static int branches(void)
{
    UmcExecutionBounds bounds;
    CHECK(umc_execution_bounds(diamond,8,&bounds,NULL)==UMC_OK);
    CHECK(bounds.minimum_instructions==4 && bounds.maximum_instructions==7);
    CHECK(bounds.maximum_instructions<8);
    return 0;
}
static int fuel(void)
{
    UmcInstruction program[8];memcpy(program,diamond,sizeof program);
    UmcExecutionBounds bounds;UmcResult result={99,99};
    CHECK(umc_execution_bounds(program,8,&bounds,NULL)==UMC_OK);
    CHECK(umc_execute(program,8,bounds.maximum_instructions,&result,NULL)==UMC_OK);
    CHECK(result.value==42 && result.instructions_executed==7);
    result=(UmcResult){99,99};
    CHECK(umc_execute(program,8,bounds.maximum_instructions-1U,&result,NULL)==UMC_FUEL_EXHAUSTED);
    CHECK(result.value==99 && result.instructions_executed==99);
    program[0].operand=0;
    CHECK(umc_execute(program,8,bounds.minimum_instructions,&result,NULL)==UMC_OK);
    CHECK(result.value==7 && result.instructions_executed==4);
    return 0;
}
/* This oracle walks every structural path forward and counts visited records.
 * It intentionally does not use the production reverse dynamic program. */
static void enumerate(const UmcInstruction *program,size_t index,size_t visited,size_t *low,size_t *high)
{
    ++visited;
    if(program[index].opcode==UMC_RETURN){
        if(visited<*low)*low=visited;
        if(visited>*high)*high=visited;
        return;
    }
    if(program[index].opcode==UMC_JUMP){
        enumerate(program,(size_t)program[index].operand,visited,low,high);
        return;
    }
    if(program[index].opcode==UMC_JUMP_IF_ZERO)
        enumerate(program,(size_t)program[index].operand,visited,low,high);
    enumerate(program,index+1U,visited,low,high);
}
static int source_paths(void)
{
    const char *sources[]={
        "int main(void){return 42;}",
        "int main(void){return 1 ? 40+2 : 7;}",
        "int main(void){int a=1,b=2; if(a){if(b){a+=40;}else{a=7;}}else{a=9;}return a;}",
        "int main(void){int a=1; if(a){a=2;}else if(a-1){a=3;}else{a=4;} return a;}",
        "int main(void){return (0 && (1/0)) || (1 ? 42 : 7+8);}",
        "int main(void){int a=1;{int b=2;if(a&&b){a=b*3;}else{a=b+4;}}return a;}"
    };
    for(size_t i=0;i<sizeof sources/sizeof sources[0];++i){
        UmcInstruction program[UMC_PROGRAM_LIMIT];size_t count=0;
        UmcExecutionBounds bounds;UmcResult result;
        CHECK(umc_compile(sources[i],strlen(sources[i]),program,UMC_PROGRAM_LIMIT,&count,NULL)==UMC_OK);
        CHECK(umc_execution_bounds(program,count,&bounds,NULL)==UMC_OK);
        size_t low=SIZE_MAX,high=0;enumerate(program,0,0,&low,&high);
        CHECK(bounds.minimum_instructions==low && bounds.maximum_instructions==high);
        CHECK(low<=high && high<=count);
        CHECK(umc_execute(program,count,high,&result,NULL)==UMC_OK);
        CHECK(result.instructions_executed>=low && result.instructions_executed<=high);
    }
    return 0;
}
static int conservative(void)
{
    /* The zero condition makes the long path infeasible, but structural
     * budgeting must not evaluate or optimize away that path. */
    UmcInstruction program[8];memcpy(program,diamond,sizeof program);program[0].operand=0;
    UmcExecutionBounds bounds;
    CHECK(umc_execution_bounds(program,8,&bounds,NULL)==UMC_OK);
    CHECK(bounds.minimum_instructions==4 && bounds.maximum_instructions==7);
    return 0;
}
static int arithmetic(void)
{
    const UmcInstruction program[]={{UMC_PUSH,1,0},{UMC_PUSH,0,1},{UMC_DIVIDE,0,2},{UMC_RETURN,0,3}};
    UmcExecutionBounds bounds;UmcResult result={99,99};UmcDiagnostic diagnostic;
    CHECK(umc_execution_bounds(program,4,&bounds,&diagnostic)==UMC_OK);
    CHECK(bounds.minimum_instructions==4 && bounds.maximum_instructions==4);
    CHECK(umc_execute(program,4,4,&result,&diagnostic)==UMC_ARITHMETIC_ERROR);
    CHECK(diagnostic.source_offset==2 && result.value==99 && result.instructions_executed==99);
    return 0;
}
static int refusal(void)
{
    UmcExecutionBounds bounds={91,92};UmcDiagnostic diagnostic;
    CHECK(umc_execution_bounds(NULL,8,&bounds,&diagnostic)==UMC_BAD_BYTECODE);
    CHECK(bounds.minimum_instructions==91 && bounds.maximum_instructions==92);
    CHECK(umc_execution_bounds(diamond,0,&bounds,&diagnostic)==UMC_BAD_BYTECODE);
    CHECK(bounds.minimum_instructions==91 && bounds.maximum_instructions==92);
    CHECK(umc_execution_bounds(diamond,UMC_PROGRAM_LIMIT+1U,&bounds,&diagnostic)==UMC_BAD_BYTECODE);
    CHECK(bounds.minimum_instructions==91 && bounds.maximum_instructions==92);
    CHECK(umc_execution_bounds(diamond,8,NULL,&diagnostic)==UMC_INVALID_ARGUMENT);
    return 0;
}
static int malformed(void)
{
    for(unsigned kind=0;kind<8;++kind){
        UmcInstruction program[8];memcpy(program,diamond,sizeof program);
        if(kind==0)program[1].operand=1; /* self edge */
        if(kind==1)program[1].operand=-1;
        if(kind==2)program[1].operand=8;
        if(kind==3)program[4].opcode=(UmcOpcode)255;
        if(kind==4)program[4].operand=1;
        if(kind==5)program[7].opcode=UMC_NEGATE;
        if(kind==6)program[6].opcode=UMC_LOAD_LOCAL; /* not initialized */
        if(kind==7)program[6].source_offset=UMC_SOURCE_LIMIT+1U;
        UmcExecutionBounds bounds={91,92};UmcDiagnostic expected,actual;
        const UmcStatus status=umc_validate(program,8,&expected);
        CHECK(status!=UMC_OK);
        CHECK(umc_execution_bounds(program,8,&bounds,&actual)==status);
        CHECK(actual.source_offset==expected.source_offset);
        CHECK(bounds.minimum_instructions==91 && bounds.maximum_instructions==92);
    }
    return 0;
}
static int maximum(void)
{
    UmcInstruction program[UMC_PROGRAM_LIMIT];
    program[0]=(UmcInstruction){UMC_PUSH,1,0};
    for(size_t i=1;i+1U<UMC_PROGRAM_LIMIT;++i)program[i]=(UmcInstruction){UMC_NEGATE,0,i};
    program[UMC_PROGRAM_LIMIT-1U]=(UmcInstruction){UMC_RETURN,0,UMC_PROGRAM_LIMIT-1U};
    UmcExecutionBounds bounds;UmcResult result;
    CHECK(umc_execution_bounds(program,UMC_PROGRAM_LIMIT,&bounds,NULL)==UMC_OK);
    CHECK(bounds.minimum_instructions==UMC_PROGRAM_LIMIT && bounds.maximum_instructions==UMC_PROGRAM_LIMIT);
    CHECK(umc_execute(program,UMC_PROGRAM_LIMIT,bounds.maximum_instructions,&result,NULL)==UMC_OK);
    CHECK(result.instructions_executed==UMC_PROGRAM_LIMIT);
    return 0;
}
static int wire(void)
{
    uint8_t bytes[UMC_BYTECODE_LIMIT];size_t used=0,count=0;
    UmcInstruction decoded[UMC_PROGRAM_LIMIT];UmcExecutionBounds before,after;
    CHECK(umc_encode(diamond,8,bytes,sizeof bytes,&used,NULL)==UMC_OK);
    CHECK(umc_decode(bytes,used,decoded,UMC_PROGRAM_LIMIT,&count,NULL)==UMC_OK);
    CHECK(umc_execution_bounds(diamond,8,&before,NULL)==UMC_OK);
    CHECK(umc_execution_bounds(decoded,count,&after,NULL)==UMC_OK);
    CHECK(before.minimum_instructions==after.minimum_instructions && before.maximum_instructions==after.maximum_instructions);
/* Compare the wire contract fields, not implementation-defined structure padding; retain the draft byte comparison for review. The former implementation is retained for owner review. */
#if 0
    CHECK(count==8 && memcmp(decoded,diamond,sizeof diamond)==0);
#endif
    CHECK(count==8);
    for(size_t i=0;i<count;++i){
        CHECK(decoded[i].opcode==diamond[i].opcode);
        CHECK(decoded[i].operand==diamond[i].operand);
        CHECK(decoded[i].source_offset==diamond[i].source_offset);
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"straight")==0)return straight();
    if(strcmp(argv[1],"branches")==0)return branches();
    if(strcmp(argv[1],"fuel")==0)return fuel();
    if(strcmp(argv[1],"source-paths")==0)return source_paths();
    if(strcmp(argv[1],"conservative")==0)return conservative();
    if(strcmp(argv[1],"arithmetic")==0)return arithmetic();
    if(strcmp(argv[1],"refusal")==0)return refusal();
    if(strcmp(argv[1],"malformed")==0)return malformed();
    if(strcmp(argv[1],"maximum")==0)return maximum();
    if(strcmp(argv[1],"wire")==0)return wire();
    return 2;
}
