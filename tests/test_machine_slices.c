/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_machine_slices.c
 *
 * PURPOSE:
 *   Verify bounded scheduling, fault prefixes and parity with complete execution.
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
static const UmcInstruction input[] = {
    {UMC_PUSH,7,11U},{UMC_PUSH,6,13U},{UMC_MULTIPLY,0,12U},{UMC_RETURN,0,0U}
};
static int sizes(void)
{
    for(size_t budget=1;budget<=UMC_PROGRAM_LIMIT;++budget) {
        UmcMachine m; UmcDiagnostic d; UmcSliceResult report; size_t total=0;
        CHECK(umc_machine_init(&m,input,4,4,&d)==UMC_OK);
        while(!m.frame.finished) {
            CHECK(umc_machine_run_slice(&m,budget,&report,NULL,&d)==UMC_OK);
            CHECK(report.instructions_executed>0 && report.instructions_executed<=budget);
            total+=report.instructions_executed;
            CHECK(report.finished==m.frame.finished);
        }
        UmcResult result;
        CHECK(total==4 && umc_machine_result(&m,&result,&d)==UMC_OK && result.value==42);
        UmcMachine before=m; UmcTraceStep step; memset(&step,0x55,sizeof step); UmcTraceStep prior=step;
        CHECK(umc_machine_run_slice(&m,budget,&report,&step,&d)==UMC_OK);
        CHECK(report.instructions_executed==0 && report.finished==1);
        CHECK(memcmp(&before,&m,sizeof m)==0 && memcmp(&step,&prior,sizeof step)==0);
    }
    return 0;
}
static int faults(void)
{
    const UmcInstruction failing[][4]={
        {{UMC_PUSH,1,0},{UMC_PUSH,0,1},{UMC_DIVIDE,0,2},{UMC_RETURN,0,3}},
        {{UMC_PUSH,INT32_MAX,0},{UMC_PUSH,1,1},{UMC_ADD,0,2},{UMC_RETURN,0,3}},
        {{UMC_PUSH,INT32_MIN,0},{UMC_PUSH,-1,1},{UMC_REMAINDER,0,2},{UMC_RETURN,0,3}}
    };
    for(size_t n=0;n<3;++n) {
        UmcMachine m; UmcDiagnostic d; UmcTraceStep step; UmcSliceResult report;
        CHECK(umc_machine_init(&m,failing[n],4,4,&d)==UMC_OK);
        CHECK(umc_machine_run_slice(&m,4,&report,&step,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(report.instructions_executed==2 && !report.finished && m.frame.pc==2);
        CHECK(step.instruction_index==2 && step.status==UMC_ARITHMETIC_ERROR && d.source_offset==2);
        UmcMachine before=m;
        CHECK(umc_machine_run_slice(&m,4,&report,NULL,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(report.instructions_executed==0 && memcmp(&m,&before,sizeof m)==0);
    }
    return 0;
}
static int fuel(void)
{
    for(size_t allowed=0;allowed<4;++allowed) {
        UmcMachine m; UmcDiagnostic d; UmcSliceResult report;
        CHECK(umc_machine_init(&m,input,4,allowed,&d)==UMC_OK);
        CHECK(umc_machine_run_slice(&m,4,&report,NULL,&d)==UMC_FUEL_EXHAUSTED);
        CHECK(report.instructions_executed==allowed && !report.finished);
        UmcMachine before=m;
        CHECK(umc_machine_run_slice(&m,1,&report,NULL,&d)==UMC_FUEL_EXHAUSTED);
        CHECK(report.instructions_executed==0 && memcmp(&before,&m,sizeof m)==0);
    }
    return 0;
}
static int refusal(void)
{
    UmcMachine m; UmcDiagnostic d; UmcSliceResult report={77,88};
    CHECK(umc_machine_init(&m,input,4,4,&d)==UMC_OK);
    UmcMachine before=m;
    CHECK(umc_machine_run_slice(NULL,1,&report,NULL,&d)==UMC_INVALID_ARGUMENT);
    CHECK(umc_machine_run_slice(&m,0,&report,NULL,&d)==UMC_INVALID_ARGUMENT);
    CHECK(umc_machine_run_slice(&m,SIZE_MAX,&report,NULL,&d)==UMC_INVALID_ARGUMENT);
    CHECK(umc_machine_run_slice(&m,1,NULL,NULL,&d)==UMC_INVALID_ARGUMENT);
    CHECK(report.instructions_executed==77 && report.finished==88 && memcmp(&m,&before,sizeof m)==0);
    m.program[0].opcode=(UmcOpcode)999; before=m;
    CHECK(umc_machine_run_slice(&m,1,&report,NULL,&d)==UMC_BAD_BYTECODE);
    CHECK(report.instructions_executed==77 && report.finished==88 && memcmp(&m,&before,sizeof m)==0);
    CHECK(umc_machine_init(&m,input,4,4,&d)==UMC_OK);
    m.frame.pc=m.frame.count; before=m;
    CHECK(umc_machine_run_slice(&m,1,&report,NULL,&d)==UMC_INVALID_ARGUMENT);
    CHECK(report.instructions_executed==77 && memcmp(&m,&before,sizeof m)==0);
    return 0;
}
static int parity(void)
{
    const char *sources[]={
        "int main(void){int x=5; x++; if(x>2){x*=7;}else{x/=0;}return x;}",
        "int main(void){return 0 && (1/0);}",
        "int main(void){return 1 ? 42 : 1/0;}",
        "int main(void){return (~5 & 255) ^ (7 | 16);}",
        "int main(void){return -13 % 5;}"
    };
    for(size_t n=0;n<sizeof sources/sizeof sources[0];++n) {
        UmcInstruction program[UMC_PROGRAM_LIMIT]; size_t count=0; UmcDiagnostic d; UmcResult expected,actual;
        CHECK(umc_compile(sources[n],strlen(sources[n]),program,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(program,count,UMC_PROGRAM_LIMIT,&expected,&d)==UMC_OK);
        for(size_t budget=1;budget<=count+1;++budget) {
            UmcMachine m; UmcSliceResult report;
            CHECK(umc_machine_init(&m,program,count,UMC_PROGRAM_LIMIT,&d)==UMC_OK);
            while(!m.frame.finished) CHECK(umc_machine_run_slice(&m,budget,&report,NULL,&d)==UMC_OK);
            CHECK(umc_machine_result(&m,&actual,&d)==UMC_OK);
            CHECK(actual.value==expected.value && actual.instructions_executed==expected.instructions_executed);
        }
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"sizes")==0)return sizes();
    if(strcmp(argv[1],"faults")==0)return faults();
    if(strcmp(argv[1],"fuel")==0)return fuel();
    if(strcmp(argv[1],"refusal")==0)return refusal();
    if(strcmp(argv[1],"parity")==0)return parity();
    return 2;
}
