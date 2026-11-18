/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_shifts.c
 *
 * PURPOSE:
 *   Qualify checked shifts, independent wire records and failure-preserving VM execution.
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
#include "umicom_c/machine.h"
#include <limits.h>
#include <inttypes.h>

static int Run(const char *body, int32_t expected)
{
    char source[512];
    int length = snprintf(source, sizeof source, "int main(void){%s}", body);
    CHECK(length > 0 && (size_t)length < sizeof source);
    UmcInstruction program[UMC_PROGRAM_LIMIT], decoded[UMC_PROGRAM_LIMIT];
    UmcDiagnostic diagnostic; UmcResult result;
    uint8_t bytes[UMC_BYTECODE_LIMIT];
    size_t count = 0U, size = 0U, read = 0U;
    CHECK(umc_compile(source, (size_t)length, program, UMC_PROGRAM_LIMIT,
        &count, &diagnostic) == UMC_OK);
    CHECK(umc_execute(program, count, count, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == expected);
    CHECK(umc_encode(program, count, bytes, sizeof bytes, &size, &diagnostic) == UMC_OK);
    CHECK(bytes[4] == 7U);
    CHECK(umc_decode(bytes, size, decoded, UMC_PROGRAM_LIMIT, &read, &diagnostic) == UMC_OK);
    CHECK(umc_execute(decoded, read, read, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == expected);
    return 0;
}
static int Values(void)
{
    CHECK(Run("return 21<<1;",42)==0);
    CHECK(Run("return 84>>1;",42)==0);
    CHECK(Run("return 0<<31;",0)==0);
    CHECK(Run("return 2147483647>>31;",0)==0);
    CHECK(Run("return 2147483647<<0;",INT32_MAX)==0);
    CHECK(Run("return 2147483647>>0;",INT32_MAX)==0);
    CHECK(Run("return 1<<30;",1073741824)==0);
    CHECK(Run("return 1073741823<<1;",2147483646)==0);
    CHECK(Run("return 0b10101<<0x1;",42)==0);
    CHECK(Run("return 85>>1;",42)==0);
    return 0;
}
static int Precedence(void)
{
    CHECK(Run("return 1+2<<3;",24)==0);
    CHECK(Run("return 1<<2+3;",32)==0);
    CHECK(Run("return 2<<1*3;",16)==0);
    CHECK(Run("return 64>>2>>1;",8)==0);
    CHECK(Run("return 2<<3>>1;",8)==0);
    CHECK(Run("return 2<<3<17;",1)==0);
    CHECK(Run("return 17>2<<3;",1)==0);
    CHECK(Run("return 2<<3==16;",1)==0);
    CHECK(Run("return 3<<2&8;",8)==0);
    CHECK(Run("return 3|2<<3;",19)==0);
    CHECK(Run("return 1?21<<1:0;",42)==0);
    CHECK(Run("return 0&&(1<<32);",0)==0);
    CHECK(Run("return 1||(1<<32);",1)==0);
    CHECK(Run("return 21/*first*/<</*second*/1;",42)==0);
    return 0;
}
static int Updates(void)
{
    CHECK(Run("int x=21;x<<=1;return x;",42)==0);
    CHECK(Run("int x=84;x>>=1;return x;",42)==0);
    CHECK(Run("int x=2;x<<=x;return x;",8)==0);
    CHECK(Run("int x=21;x<<=0?1/0:1;return x;",42)==0);
    CHECK(Run("int x=21;if(0){x<<=32;}else{x<<=1;}return x;",42)==0);
    CHECK(Run("int x=0;{int y=84;y>>=1;x=y;}return x;",42)==0);
    return 0;
}
static int Refusals(void)
{
    const char *bodies[] = {
        "return <<1;", "return >>1;", "return 1<<;", "return 1>>;",
        "return 1<<<2;", "return 1>>>2;", "return 1< <2;", "return 1> >2;",
        "return 1</*gap*/<2;", "return 1>/*gap*/>2;", "return 1<<=2;",
        "int x<<=1;return x;", "int x=1;x<< =2;return x;",
        "int x=1;x>>/*gap*/=2;return x;", "int x=1;return x>>=1;",
        "const int x=1;x<<=1;return x;", "int const x=1;x>>=1;return x;",
        "missing<<=1;return 0;", "return 0&&(1<<missing);"
    };
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i) {
        char source[256];
        int n=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(n>0 && (size_t)n<sizeof source);
        UmcInstruction program[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=73U;
        CHECK(umc_compile(source,(size_t)n,program,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
        CHECK(count==0U && d.source_offset<=(size_t)n);
    }
    return 0;
}
static int Faults(void)
{
    const char *bodies[] = {
        "return 1<<-1;", "return 1>>-1;", "return 1<<32;", "return 1>>32;",
        "return 0<<2147483647;", "return 0>>2147483647;",
        "return -1<<0;", "return -1>>0;", "return ~0<<1;",
        "return (-2147483647-1)>>31;", "return 1<<31;",
        "return 1073741824<<1;", "int x=1;x<<=31;return x;"
    };
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i) {
        char source[256];int n=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(n>0 && (size_t)n<sizeof source);
        UmcInstruction p[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=0U;
        CHECK(umc_compile(source,(size_t)n,p,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        UmcResult result={73,91U};UmcTraceStep trace[UMC_PROGRAM_LIMIT];size_t used=0U;
        CHECK(umc_execute_trace(p,count,count,&result,trace,UMC_PROGRAM_LIMIT,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(result.value==73 && result.instructions_executed==91U && used>0U);
        const char *operator_at=strstr(source,"<<");
        if(operator_at==NULL)operator_at=strstr(source,">>");
        CHECK(d.source_offset==(size_t)(operator_at-source));
        CHECK(trace[used-1U].status==UMC_ARITHMETIC_ERROR);
        CHECK(trace[used-1U].stack_depth==2U);
        UmcMachine machine;CHECK(umc_machine_init(&machine,p,count,count,&d)==UMC_OK);
        for(;;) {
            UmcMachine before=machine;UmcTraceStep step;
            UmcStatus status=umc_machine_step(&machine,&step,&d);
            if(status!=UMC_OK) {
                CHECK(status==UMC_ARITHMETIC_ERROR);
                CHECK(!memcmp(&before,&machine,sizeof machine));
                CHECK(umc_machine_step(&machine,&step,&d)==UMC_ARITHMETIC_ERROR);
                CHECK(!memcmp(&before,&machine,sizeof machine));
                break;
            }
            CHECK(!machine.frame.finished);
        }
    }
    return 0;
}
/* Literal bytes independently fix opcode identity, byte order and locations. */
static const uint8_t golden[]={
    'U','M','C','B',7,0,4,0,
    1,21,0,0,0,10,0,0,0,
    1,1,0,0,0,20,0,0,0,
    22,0,0,0,0,30,0,0,0,
    7,0,0,0,0,40,0,0,0
};
static int Wire(void)
{
    CHECK((int)UMC_SHIFT_LEFT==22 && (int)UMC_SHIFT_RIGHT==23);
    CHECK(!strcmp(umc_opcode_text(UMC_SHIFT_LEFT),"shift-left"));
    CHECK(!strcmp(umc_opcode_text(UMC_SHIFT_RIGHT),"shift-right"));
    for(unsigned op=22U;op<=23U;++op) {
        uint8_t bytes[sizeof golden],encoded[sizeof golden];
        memcpy(bytes,golden,sizeof bytes);bytes[26]=(uint8_t)op;
        UmcInstruction p[4];UmcDiagnostic d;UmcResult r;size_t count=0U,size=0U;
        CHECK(umc_decode(bytes,sizeof bytes,p,4U,&count,&d)==UMC_OK);
        CHECK(umc_execute(p,count,count,&r,&d)==UMC_OK);
        CHECK(r.value==(op==22U?42:10));
        CHECK(umc_encode(p,count,encoded,sizeof encoded,&size,&d)==UMC_OK);
        CHECK(size==sizeof bytes && !memcmp(encoded,bytes,size));
        UmcProgramInfo info;CHECK(umc_inspect(p,count,&info,&d)==UMC_OK);
        CHECK(info.peak_stack==2U && info.arithmetic_count==0U);
        for(unsigned version=0U;version<9U;++version)if(version!=7U) {
            bytes[4]=(uint8_t)version;count=71U;
            CHECK(umc_decode(bytes,sizeof bytes,p,4U,&count,&d)==UMC_BAD_BYTECODE);
            CHECK(count==0U);
        }
    }
    return 0;
}
static int Versions(void)
{
    UmcInstruction p[]={{UMC_PUSH,21,0U},{UMC_PUSH,1,1U},{UMC_BITWISE_OR,0,2U},
        {UMC_PUSH,1,3U},{UMC_SHIFT_LEFT,0,4U},{UMC_RETURN,0,5U}};
    uint8_t bytes[128];UmcDiagnostic d;size_t size=0U,count=0U;
    CHECK(umc_encode(p,6U,bytes,sizeof bytes,&size,&d)==UMC_OK && bytes[4]==7U);
    p[2].opcode=UMC_SHIFT_LEFT;p[4].opcode=UMC_BITWISE_OR;
    CHECK(umc_encode(p,6U,bytes,sizeof bytes,&size,&d)==UMC_OK && bytes[4]==7U);
    p[2].opcode=UMC_REMAINDER;
    CHECK(umc_encode(p,6U,bytes,sizeof bytes,&size,&d)==UMC_OK && bytes[4]==6U);
    p[2].opcode=UMC_BITWISE_OR;p[4].opcode=UMC_REMAINDER;
    CHECK(umc_encode(p,6U,bytes,sizeof bytes,&size,&d)==UMC_OK && bytes[4]==6U);
    bytes[4]=7U;
    CHECK(umc_decode(bytes,size,p,6U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_CHECKED_SHIFTS);
    CHECK(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_CHECKED_SHIFTS);
    CHECK(umc_runtime_profile(UMC_PROFILE_WIRE_VERSIONS)==254U);
    return 0;
}
static int Malformed(void)
{
    UmcInstruction p[4];UmcDiagnostic d;size_t count=0U;
    uint8_t bytes[sizeof golden];
    for(size_t length=0U;length<sizeof golden;++length)
        CHECK(umc_decode(golden,length,p,4U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    memcpy(bytes,golden,sizeof bytes);bytes[27]=1U;
    CHECK(umc_decode(bytes,sizeof bytes,p,4U,&count,&d)==UMC_BAD_BYTECODE);
    memcpy(bytes,golden,sizeof bytes);bytes[26]=24U;
    CHECK(umc_decode(bytes,sizeof bytes,p,4U,&count,&d)==UMC_BAD_BYTECODE);
    CHECK(umc_decode(golden,sizeof golden,p,4U,&count,&d)==UMC_OK);
    p[1].opcode=UMC_SHIFT_LEFT;p[1].operand=0;
    UmcResult result={73,91U};
    CHECK(umc_execute(p,4U,4U,&result,&d)==UMC_BAD_BYTECODE);
    CHECK(result.value==73 && result.instructions_executed==91U);
    return 0;
}
static int Fuel(void)
{
    UmcInstruction p[4];UmcDiagnostic d;size_t count=0U;
    CHECK(umc_decode(golden,sizeof golden,p,4U,&count,&d)==UMC_OK);
    for(size_t fuel=0U;fuel<4U;++fuel) {
        UmcResult r={73,91U};UmcTraceStep trace[4];size_t used=0U;
        CHECK(umc_execute_trace(p,count,fuel,&r,trace,4U,&used,&d)==UMC_FUEL_EXHAUSTED);
        CHECK(r.value==73 && r.instructions_executed==91U && used==fuel+1U);
    }
    UmcMachine machine;CHECK(umc_machine_init(&machine,p,4U,4U,&d)==UMC_OK);
    for(size_t i=0U;i<4U;++i) {
        UmcSliceResult slice;UmcTraceStep step;
        CHECK(umc_machine_run_slice(&machine,1U,&slice,&step,&d)==UMC_OK);
        CHECK(slice.instructions_executed==1U && slice.finished==(i==3U));
    }
    UmcResult r;CHECK(umc_machine_result(&machine,&r,&d)==UMC_OK && r.value==42);
    return 0;
}
static int Spans(void)
{
    const char source[]="int main(void){return 21<<1;}";
    char span[sizeof source-1U];memcpy(span,source,sizeof span);
    const size_t offset=(size_t)(strstr(source,"<<")-source);
    UmcInstruction p[4];UmcDiagnostic d;size_t count=0U;
    CHECK(umc_compile(span,sizeof span,p,4U,&count,&d)==UMC_OK);
    CHECK(count==4U && p[2].source_offset==offset && p[2].opcode==UMC_SHIFT_LEFT);
    CHECK(umc_compile(span,offset+1U,p,4U,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    CHECK(umc_compile(span,offset+2U,p,4U,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    CHECK(umc_compile(span,sizeof span,p,3U,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    return 0;
}
static int Native(void)
{
    /* Compare only defined, representable native C operations. Every count is
     * exercised; overflow and negative operands belong to refusal tests. */
    for(unsigned shift=0U;shift<32U;++shift) {
        uint32_t ceiling=(uint32_t)INT32_MAX>>shift;
        uint32_t values[]={0U,ceiling/2U,ceiling};
        for(size_t i=0U;i<3U;++i) {
            char body[128];
            int n=snprintf(body,sizeof body,"return %" PRIu32 "<<%u;",values[i],shift);
            CHECK(n>0 && (size_t)n<sizeof body);
            CHECK(Run(body,(int32_t)(values[i]<<shift))==0);
            n=snprintf(body,sizeof body,"return 2147483647>>%u;",shift);
            CHECK(n>0 && (size_t)n<sizeof body);
            CHECK(Run(body,INT32_MAX>>shift)==0);
        }
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"values"))return Values();
    if(!strcmp(argv[1],"precedence"))return Precedence();
    if(!strcmp(argv[1],"updates"))return Updates();
    if(!strcmp(argv[1],"refusals"))return Refusals();
    if(!strcmp(argv[1],"faults"))return Faults();
    if(!strcmp(argv[1],"wire"))return Wire();
    if(!strcmp(argv[1],"versions"))return Versions();
    if(!strcmp(argv[1],"malformed"))return Malformed();
    if(!strcmp(argv[1],"fuel"))return Fuel();
    if(!strcmp(argv[1],"spans"))return Spans();
    if(!strcmp(argv[1],"native"))return Native();
    return 2;
}
