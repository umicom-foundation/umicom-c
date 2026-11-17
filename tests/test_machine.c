/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_machine.c
 *
 * PURPOSE:
 *   Exercise pause, resume, borrowed-input release and unchanged state after rejected VM steps.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
#include "umicom_c/machine.h"
#include <limits.h>
static const UmcInstruction simple[] = {
    {UMC_PUSH, 7, 10}, {UMC_PUSH, 6, 12}, {UMC_MULTIPLY, 0, 11}, {UMC_RETURN, 0, 3}
};
static int sequence(void)
{
    UmcInstruction input[4]; memcpy(input, simple, sizeof input);
    UmcMachine m; UmcDiagnostic d; UmcTraceStep s;
    CHECK(umc_machine_init(&m,input,4,4,&d)==UMC_OK);
    input[0].operand=99; /* Admission owns a copy. */
    const int32_t expected[]={7,6,42,42};
    for(size_t i=0;i<4;++i) {
        CHECK(m.frame.executed==i && m.frame.pc==i);
        CHECK(umc_machine_step(&m,&s,&d)==UMC_OK);
        CHECK(s.instruction_index==i && s.top==expected[i]);
        CHECK(m.frame.finished==(i==3));
    }
    UmcResult result={-9,99};
    CHECK(umc_machine_result(&m,&result,&d)==UMC_OK && result.value==42 && result.instructions_executed==4);
    UmcMachine before=m; memset(&s,0x55,sizeof s); UmcTraceStep prior=s;
    CHECK(umc_machine_step(&m,&s,&d)==UMC_INVALID_ARGUMENT);
    CHECK(memcmp(&m,&before,sizeof m)==0 && memcmp(&s,&prior,sizeof s)==0);
    return 0;
}
static int faults(void)
{
    const UmcInstruction cases[][4]={
        {{UMC_PUSH,1,0},{UMC_PUSH,0,1},{UMC_DIVIDE,0,2},{UMC_RETURN,0,3}},
        {{UMC_PUSH,INT32_MAX,0},{UMC_PUSH,1,1},{UMC_ADD,0,2},{UMC_RETURN,0,3}},
        {{UMC_PUSH,INT32_MIN,0},{UMC_PUSH,-1,1},{UMC_REMAINDER,0,2},{UMC_RETURN,0,3}}
    };
    for(size_t n=0;n<3;++n) {
        UmcMachine m; UmcDiagnostic d; UmcTraceStep s;
        CHECK(umc_machine_init(&m,cases[n],4,4,&d)==UMC_OK);
        CHECK(umc_machine_step(&m,NULL,&d)==UMC_OK);
        CHECK(umc_machine_step(&m,NULL,&d)==UMC_OK);
        UmcMachine before=m;
        CHECK(umc_machine_step(&m,&s,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(s.instruction_index==2 && s.stack_depth==2 && s.source_offset==2);
        CHECK(memcmp(&m,&before,sizeof m)==0);
        CHECK(umc_machine_step(&m,NULL,&d)==UMC_ARITHMETIC_ERROR);
        UmcResult result={99,88};
        CHECK(umc_machine_result(&m,&result,&d)==UMC_INVALID_ARGUMENT && result.value==99 && result.instructions_executed==88);
    }
    return 0;
}
static int fuel(void)
{
    for(size_t budget=0;budget<4;++budget) {
        UmcMachine m; UmcDiagnostic d; UmcTraceStep s;
        CHECK(umc_machine_init(&m,simple,4,budget,&d)==UMC_OK);
        for(size_t i=0;i<budget;++i) CHECK(umc_machine_step(&m,NULL,&d)==UMC_OK);
        UmcMachine before=m;
        CHECK(umc_machine_step(&m,&s,&d)==UMC_FUEL_EXHAUSTED);
        CHECK(s.instruction_index==budget && memcmp(&before,&m,sizeof m)==0);
    }
    return 0;
}
static int refusal(void)
{
    UmcMachine m; memset(&m,0x55,sizeof m); UmcMachine before=m; UmcDiagnostic d;
    CHECK(umc_machine_init(&m,NULL,4,4,&d)==UMC_BAD_BYTECODE && memcmp(&m,&before,sizeof m)==0);
    CHECK(umc_machine_init(NULL,simple,4,4,&d)==UMC_INVALID_ARGUMENT);
    CHECK(umc_machine_step(NULL,NULL,&d)==UMC_INVALID_ARGUMENT);
    CHECK(umc_machine_init(&m,simple,4,4,&d)==UMC_OK);
    m.frame.pc=UMC_PROGRAM_LIMIT; before=m;
    CHECK(umc_machine_step(&m,NULL,&d)==UMC_INVALID_ARGUMENT && memcmp(&m,&before,sizeof m)==0);
    CHECK(umc_machine_init(&m,simple,4,4,&d)==UMC_OK);
    m.frame.depth=UMC_STACK_LIMIT+1;
    CHECK(umc_machine_step(&m,NULL,&d)==UMC_INVALID_ARGUMENT);
    CHECK(umc_machine_init(&m,simple,4,4,&d)==UMC_OK);
    m.program[0].opcode=(UmcOpcode)999;
    CHECK(umc_machine_step(&m,NULL,&d)==UMC_BAD_BYTECODE);
    return 0;
}
static int parity(void)
{
    const char *cases[]={
        "int main(void){int a=5; a++; if(a>2){a*=7;}else{a/=0;}return a;}",
        "int main(void){return 0 && (1/0);}",
        "int main(void){return 1 ? 42 : 1/0;}",
        "int main(void){return (~5 & 255) ^ (7 | 16);}",
        "int main(void){return -13 % 5;}",
        "int main(void){return 1/0;}"
    };
    const int32_t expected[]={42,0,42,237,-3,0};
    for(size_t n=0;n<sizeof cases/sizeof cases[0];++n) {
        UmcInstruction p[UMC_PROGRAM_LIMIT]; size_t count=0,used=0;
        UmcDiagnostic d; UmcResult r={99,99},t={99,99},single={99,99}; UmcTraceStep trace[UMC_PROGRAM_LIMIT];
        CHECK(umc_compile(cases[n],strlen(cases[n]),p,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        UmcStatus status=umc_execute(p,count,count,&r,&d);
        CHECK(umc_execute_trace(p,count,count,&t,trace,UMC_PROGRAM_LIMIT,&used,&d)==status);
        UmcMachine m; CHECK(umc_machine_init(&m,p,count,count,&d)==UMC_OK);
        for(size_t i=0;i<used;++i) {
            UmcTraceStep s; UmcStatus step=umc_machine_step(&m,&s,&d);
            CHECK(step==trace[i].status && s.instruction_index==trace[i].instruction_index &&
                s.source_offset==trace[i].source_offset && s.top==trace[i].top && s.stack_depth==trace[i].stack_depth);
            if(step!=UMC_OK) CHECK(i+1==used);
        }
        if(status==UMC_OK) {
            CHECK(umc_machine_result(&m,&single,&d)==UMC_OK);
            CHECK(single.value==expected[n] && single.value==r.value && single.value==t.value);
            CHECK(single.instructions_executed==used && r.instructions_executed==used);
        } else CHECK(status==UMC_ARITHMETIC_ERROR && !m.frame.finished);
    }
    return 0;
}
static int locals(void)
{
    const UmcInstruction p[]={{UMC_PUSH,42,0},{UMC_STORE_LOCAL,31,1},{UMC_LOAD_LOCAL,31,2},{UMC_RETURN,0,3}};
    UmcMachine m; UmcDiagnostic d;
    CHECK(umc_machine_init(&m,p,4,4,&d)==UMC_OK);
    CHECK(m.frame.initialized==0 && umc_machine_step(&m,NULL,&d)==UMC_OK);
    CHECK(umc_machine_step(&m,NULL,&d)==UMC_OK && m.frame.locals[31]==42 && m.frame.initialized==UINT32_C(0x80000000));
    m.frame.initialized=0; UmcMachine before=m;
    CHECK(umc_machine_step(&m,NULL,&d)==UMC_BAD_BYTECODE && memcmp(&before,&m,sizeof m)==0);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"sequence"))return sequence();
    if(!strcmp(argv[1],"faults"))return faults();
    if(!strcmp(argv[1],"fuel"))return fuel();
    if(!strcmp(argv[1],"refusal"))return refusal();
    if(!strcmp(argv[1],"parity"))return parity();
    if(!strcmp(argv[1],"locals"))return locals();
    return 2;
}
