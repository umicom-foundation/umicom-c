/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_bitwise.c
 *
 * PURPOSE:
 *   Qualify eager signed bitwise precedence, local updates, canonical format-6 records and bounded runtime ownership.
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
#include <inttypes.h>
#include <limits.h>

static int Run(const char *body, int32_t expected)
{
    char source[512];
    int length = snprintf(source, sizeof source, "int main(void){%s}", body);
    CHECK(length > 0 && (size_t)length < sizeof source);
    UmcInstruction p[UMC_PROGRAM_LIMIT], decoded[UMC_PROGRAM_LIMIT];
    UmcDiagnostic d; UmcResult r; uint8_t bytes[UMC_BYTECODE_LIMIT];
    size_t count = 0U, size = 0U, admitted = 0U;
    CHECK(umc_compile(source, (size_t)length, p, UMC_PROGRAM_LIMIT, &count, &d) == UMC_OK);
    CHECK(umc_execute(p, count, count, &r, &d) == UMC_OK && r.value == expected);
    CHECK(umc_encode(p, count, bytes, sizeof bytes, &size, &d) == UMC_OK);
    CHECK(umc_decode(bytes, size, decoded, UMC_PROGRAM_LIMIT, &admitted, &d) == UMC_OK);
    CHECK(umc_execute(decoded, admitted, admitted, &r, &d) == UMC_OK && r.value == expected);
    return 0;
}
static int Values(void)
{
    static const struct { const char *body; int32_t value; } cases[] = {
        {"return 42&15;",10}, {"return 42^15;",37}, {"return 42|15;",47},
        {"return -1&42;",42}, {"return -1^42;",-43}, {"return -1|42;",-1},
        {"return 0&(-2147483647-1);",0},
        {"return 2147483647^(-2147483647-1);",-1},
        {"return 2147483647|(-2147483647-1);",-1},
        {"return (-2147483647-1)&-1;",INT32_MIN},
        {"return (-2147483647-1)^-1;",INT32_MAX},
        {"return (-2147483647-1)^(-2147483647-1);",0},
        {"return 0b101010&0x0f;",10}, {"return ~0&1;",1},
        {"return ~0|1;",-1}, {"return ~0^1;",-2},
        {"return 1 & !0;",1}, {"return 1 | !0;",1},
        {"return 1 & 2;",0}, {"return 1 | 2;",3}
    };
    for (size_t i=0U; i<sizeof cases/sizeof cases[0]; ++i)
        CHECK(Run(cases[i].body,cases[i].value)==0);
    return 0;
}
static int Precedence(void)
{
    static const struct { const char *body; int32_t value; } cases[] = {
        {"return 4|2&1;",4}, {"return 7^3&1;",6}, {"return 4|3^1;",6},
        {"return 7&3^1|8;",10}, {"return 7^3^1;",5}, {"return 7&3&1;",1},
        {"return 1|2|4;",7}, {"return 5&3==1;",0}, {"return (5&3)==1;",1},
        {"return 1|2==2;",1}, {"return 1|2<2;",1}, {"return 1|2+4;",7},
        {"return 1|0&&0;",0}, {"return 0||2&1;",0},
        {"return 0?99:6^3;",5}, {"return 4&1?7:9;",9},
        {"return 0&&(1|1/0);",0}, {"return 1||(1&1/0);",1},
        {"return 7/*a*/&/*b*/3;",3}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i)
        CHECK(Run(cases[i].body,cases[i].value)==0);
    return 0;
}
static int Updates(void)
{
    CHECK(Run("int x=42;x&=15;return x;",10)==0);
    CHECK(Run("int x=42;x^=15;return x;",37)==0);
    CHECK(Run("int x=42;x|=15;return x;",47)==0);
    CHECK(Run("int x=1;x&=2;return x;",0)==0);
    CHECK(Run("int x=7;x^=x;return x;",0)==0);
    CHECK(Run("int x=7;x|=x;return x;",7)==0);
    CHECK(Run("int x=-1;x&=0?1/0:42;return x;",42)==0);
    CHECK(Run("int x=0;if(0){x^=1/0;}else if(1){x|=42;}return x;",42)==0);
    CHECK(Run("int x=63;x&=~16;x^=5;x|=0;return x;",42)==0);
    return 0;
}
static int Refusals(void)
{
    const char *bodies[]={"return &1;","return |1;","return ^1;","return 1&;",
        "return 1|;","return 1^;","return 1^^2;","return 1& &2;",
        "return 1| |2;","return 1<<2;","return 1>>2;","return 1u&2;",
        "return 1.0|2;","int x&=1;return x;","int x=1;return x|=2;",
        "int x=1;x& =2;return x;","int x=1;x^/*gap*/=2;return x;",
        "int x=1;x||=2;return x;","int x=1;x&&=2;return x;",
        "x|=1;return 0;","return 0&(missing);"};
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i){
        char source[256];int length=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction p[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=91U;
        CHECK(umc_compile(source,(size_t)length,p,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
        CHECK(count==0U);
    }
    return 0;
}
static int Eager(void)
{
    const char *bodies[]={"return 0&(1/0);","return -1|(1/0);","return 0^(1/0);",
        "return (1/0)&0;","int x=0;x&=1/0;return x;"};
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i){
        char source[256];int length=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction p[UMC_PROGRAM_LIMIT];UmcTraceStep rows[UMC_PROGRAM_LIMIT];
        UmcDiagnostic d;UmcResult r={91,73U};size_t count=0U,used=0U;
        CHECK(umc_compile(source,(size_t)length,p,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute_trace(p,count,count,&r,rows,UMC_PROGRAM_LIMIT,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==91 && r.instructions_executed==73U && used>0U);
        CHECK(rows[used-1U].opcode==UMC_DIVIDE);
        CHECK(d.source_offset==(size_t)(strchr(source,'/')-source));
    }
    return 0;
}
/* Fixed independent records, not produced by the compiler under test. */
static const uint8_t golden[]={
    'U','M','C','B',6,0,4,0,
    1,255,255,255,255,10,0,0,0,
    1,42,0,0,0,20,0,0,0,
    19,0,0,0,0,30,0,0,0,
    7,0,0,0,0,40,0,0,0
};
static int Wire(void)
{
    CHECK((int)UMC_BITWISE_AND==19 && (int)UMC_BITWISE_XOR==20 && (int)UMC_BITWISE_OR==21);
    CHECK(!strcmp(umc_opcode_text(UMC_BITWISE_AND),"bitwise-and"));
    CHECK(!strcmp(umc_opcode_text(UMC_BITWISE_XOR),"bitwise-xor"));
    CHECK(!strcmp(umc_opcode_text(UMC_BITWISE_OR),"bitwise-or"));
    const int32_t expected[]={42,-43,-1};
    for(unsigned op=19U;op<=21U;++op){
        uint8_t wire[sizeof golden],encoded[sizeof golden];memcpy(wire,golden,sizeof wire);
        wire[26]=(uint8_t)op;UmcInstruction p[4];UmcDiagnostic d;UmcResult r;
        size_t count=0U,bytes=0U;
        CHECK(umc_decode(wire,sizeof wire,p,4U,&count,&d)==UMC_OK && count==4U);
        CHECK(umc_execute(p,count,count,&r,&d)==UMC_OK && r.value==expected[op-19U]);
        CHECK(umc_encode(p,count,encoded,sizeof encoded,&bytes,&d)==UMC_OK);
        CHECK(bytes==sizeof wire && !memcmp(wire,encoded,bytes));
        UmcProgramInfo info;CHECK(umc_inspect(p,count,&info,&d)==UMC_OK);
        CHECK(info.peak_stack==2U && info.arithmetic_count==0U);
        for(unsigned version=0U;version<=7U;++version)if(version!=6U){
            wire[4]=(uint8_t)version;count=91U;
            CHECK(umc_decode(wire,sizeof wire,p,4U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
        }
    }
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_BINARY_BITWISE);
    CHECK(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_BINARY_BITWISE);
    CHECK(umc_runtime_profile(UMC_PROFILE_WIRE_VERSIONS)==126U);
    return 0;
}
static int Versions(void)
{
    const UmcInstruction p[]={{UMC_PUSH,7,0U},{UMC_PUSH,3,1U},{UMC_REMAINDER,0,2U},
        {UMC_PUSH,2,3U},{UMC_BITWISE_OR,0,4U},{UMC_RETURN,0,5U}};
    UmcDiagnostic d;uint8_t wire[128];size_t size=0U;
    CHECK(umc_encode(p,6U,wire,sizeof wire,&size,&d)==UMC_OK && wire[4]==6U);
    UmcInstruction q[6];memcpy(q,p,sizeof q);
    q[2].opcode=UMC_BITWISE_OR;q[4].opcode=UMC_REMAINDER;
    CHECK(umc_encode(q,6U,wire,sizeof wire,&size,&d)==UMC_OK && wire[4]==6U);
    q[2].opcode=UMC_ADD;
    CHECK(umc_encode(q,6U,wire,sizeof wire,&size,&d)==UMC_OK && wire[4]==5U);
    wire[4]=6U;size_t count=91U;
    CHECK(umc_decode(wire,62U,q,6U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    return 0;
}
static int Fuel(void)
{
    UmcInstruction p[4];UmcDiagnostic d;size_t count=0U;
    CHECK(umc_decode(golden,sizeof golden,p,4U,&count,&d)==UMC_OK);
    for(size_t fuel=0U;fuel<4U;++fuel){
        UmcResult r={91,73U};UmcTraceStep rows[4];size_t used=0U;
        CHECK(umc_execute_trace(p,count,fuel,&r,rows,4U,&used,&d)==UMC_FUEL_EXHAUSTED);
        CHECK(r.value==91 && r.instructions_executed==73U && used==fuel+1U);
        CHECK(rows[used-1U].status==UMC_FUEL_EXHAUSTED);
    }
    UmcResult r;UmcTraceStep rows[4];size_t used=0U;
    CHECK(umc_execute_trace(p,count,4U,&r,rows,4U,&used,&d)==UMC_OK);
    CHECK(r.value==42 && used==4U && rows[2].top==42 && rows[2].stack_depth==1U);
    return 0;
}
static int Malformed(void)
{
    uint8_t wire[sizeof golden];UmcInstruction p[4];UmcDiagnostic d;size_t count=91U;
    memcpy(wire,golden,sizeof wire);wire[27]=1U;
    CHECK(umc_decode(wire,sizeof wire,p,4U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    memcpy(wire,golden,sizeof wire);wire[26]=22U;
    CHECK(umc_decode(wire,sizeof wire,p,4U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    CHECK(umc_decode(golden,sizeof golden,p,4U,&count,&d)==UMC_OK);
    p[1].opcode=UMC_BITWISE_AND;p[1].operand=0;
    UmcResult r={91,73U};CHECK(umc_execute(p,4U,4U,&r,&d)==UMC_BAD_BYTECODE);
    CHECK(r.value==91 && r.instructions_executed==73U);
    return 0;
}
static int Spans(void)
{
    const char source[]="int main(void){return 42&15;}";
    char span[sizeof source-1U];memcpy(span,source,sizeof span);
    UmcInstruction p[4];UmcDiagnostic d;size_t count=0U;
    CHECK(umc_compile(span,sizeof span,p,4U,&count,&d)==UMC_OK && count==4U);
    size_t offset=(size_t)(strchr(source,'&')-source);
    CHECK(p[2].opcode==UMC_BITWISE_AND && p[2].source_offset==offset);
    CHECK(umc_compile(span,offset+1U,p,4U,&count,&d)==UMC_SYNTAX_ERROR);
    CHECK(count==0U && d.source_offset==offset+1U);
    CHECK(umc_compile(span,sizeof span,p,3U,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    return 0;
}
static int Limits(void)
{
    char source[512]="int main(void){return 0";
    size_t used=strlen(source);
    for(size_t i=0U;i<127U;++i){source[used++]='|';source[used++]='1';}
    source[used++]=';';source[used++]='}';
    UmcInstruction p[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t count=0U;UmcResult r;
    CHECK(umc_compile(source,used,p,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK && count==256U);
    CHECK(umc_execute(p,count,count,&r,&d)==UMC_OK && r.value==1);
    used-=2U;memcpy(source+used,"|1;}",4U);used+=4U;
    CHECK(umc_compile(source,used,p,UMC_PROGRAM_LIMIT,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    return 0;
}
static int Native(void)
{
    for(int32_t a=-64;a<=64;a+=7)for(int32_t b=-64;b<=64;b+=11){
        char body[128];int n=snprintf(body,sizeof body,"return (%" PRId32 ")&(%" PRId32 ");",a,b);
        CHECK(n>0 && (size_t)n<sizeof body);CHECK(Run(body,a&b)==0);
        n=snprintf(body,sizeof body,"return (%" PRId32 ")^(%" PRId32 ");",a,b);
        CHECK(n>0 && (size_t)n<sizeof body);CHECK(Run(body,a^b)==0);
        n=snprintf(body,sizeof body,"return (%" PRId32 ")|(%" PRId32 ");",a,b);
        CHECK(n>0 && (size_t)n<sizeof body);CHECK(Run(body,a|b)==0);
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
    if(!strcmp(argv[1],"eager"))return Eager();
    if(!strcmp(argv[1],"wire"))return Wire();
    if(!strcmp(argv[1],"versions"))return Versions();
    if(!strcmp(argv[1],"fuel"))return Fuel();
    if(!strcmp(argv[1],"malformed"))return Malformed();
    if(!strcmp(argv[1],"spans"))return Spans();
    if(!strcmp(argv[1],"limits"))return Limits();
    if(!strcmp(argv[1],"native"))return Native();
    return 2;
}
