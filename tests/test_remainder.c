/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_remainder.c
 *
 * PURPOSE:
 *   Qualify signed remainder, compound updates, canonical wire records, fuel and native-C agreement.
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
#include <limits.h>
#include <inttypes.h>

static int RunBody(const char *body, int32_t expected)
{
    char source[512];
    int length = snprintf(source, sizeof source, "int main(void){%s}", body);
    CHECK(length > 0 && (size_t)length < sizeof source);
    UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic diagnostic;
    UmcResult result; size_t count = 0U;
    CHECK(umc_compile(source, (size_t)length, code, UMC_PROGRAM_LIMIT, &count, &diagnostic) == UMC_OK);
    CHECK(umc_execute(code, count, count, &result, &diagnostic) == UMC_OK);
    CHECK(result.value == expected);
    return 0;
}
static int Values(void)
{
    static const struct { const char *body; int32_t value; } cases[] = {
        {"return 7%3;",1}, {"return -7%3;",-1}, {"return 7%-3;",1}, {"return -7%-3;",-1},
        {"return 0%3;",0}, {"return 7%1;",0}, {"return 7%-1;",0},
        {"return (-2147483647-1)%1;",0}, {"return (-2147483647-1)%2;",0},
        {"return (-2147483647-1)%3;",-2}, {"return (-2147483647-1)%2147483647;",-1},
        {"return 2147483647%(-2147483647-1);",INT32_MAX},
        {"return (-2147483647-1)%(-2147483647-1);",0},
        {"return 2+7%3*4;",6}, {"return 20/3%2;",0}, {"return 20%6/2;",1},
        {"return 20%6%3;",2}, {"return 7%(3*2);",1},
        {"return 0x2a%0b101;",2}, {"return 7%3==1;",1},
        {"return 0&&1%0;",0}, {"return 1||1%0;",1}, {"return 0?1%0:42;",42},
        {"int x=137;x%=95;return x;",42},
        {"int x=7;x%=x;return x;",0}, {"int x=-7;x%=-3;return x;",-1},
        {"int x=7;x%=0?1%0:3;return x;",1},
        {"int x=137;if(0){x%=0;}else if(1){x%=95;}return x;",42},
        {"int x=7;x/*value*/%=/*divisor*/3;return x;",1}
    };
    for (size_t i=0U;i<sizeof cases/sizeof cases[0];++i) CHECK(RunBody(cases[i].body,cases[i].value)==0);
    return 0;
}
static int Faults(void)
{
    const char *bodies[]={"return 1%0;","return (-2147483647-1)%-1;",
        "int x=7;x%=0;return x;","int x=(-2147483647-1);x%=-1;return x;"};
    for (size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i) {
        char source[256]; int length=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcTraceStep rows[UMC_PROGRAM_LIMIT];
        UmcDiagnostic d; UmcResult r={91,73U}; size_t count=0U,used=0U;
        CHECK(umc_compile(source,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute_trace(code,count,count,&r,rows,UMC_PROGRAM_LIMIT,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==91 && r.instructions_executed==73U);
        CHECK(used>0U && rows[used-1U].opcode==UMC_REMAINDER && rows[used-1U].stack_depth==2U);
        CHECK(rows[used-1U].status==UMC_ARITHMETIC_ERROR);
        CHECK(d.source_offset==(size_t)(strchr(source,'%')-source));
    }
    return 0;
}
static int Refusals(void)
{
    const char *bodies[]={"return %3;","return 7%;","return 7%%3;",
        "int x%=3;return x;","x%=3;return 0;","int x=7;x% =3;return x;",
        "int x=7;x%/**/=3;return x;","int x=7;return x%=3;",
        "int x=7;int y=x%=3;return y;","int x=7;int y=3;x%=y%=2;return x;",
        "int x=7;x%=missing;return x;","return 7.0%3;","return 7%3u;"};
    for (size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i) {
        char source[256]; int length=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d;size_t count=99U;
        CHECK(umc_compile(source,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
        CHECK(count==0U);
    }
    return 0;
}
static const UmcInstruction literal[]={
    {UMC_PUSH,7,10U},{UMC_PUSH,3,20U},{UMC_REMAINDER,0,30U},{UMC_RETURN,0,40U}
};
static const uint8_t wire[]={
    'U','M','C','B',5,0,4,0,
    1,7,0,0,0,10,0,0,0,
    1,3,0,0,0,20,0,0,0,
    18,0,0,0,0,30,0,0,0,
    7,0,0,0,0,40,0,0,0
};
static int Wire(void)
{
    uint8_t encoded[UMC_BYTECODE_LIMIT];UmcInstruction decoded[4];UmcDiagnostic d;size_t size=0U,count=0U;
    CHECK((int)UMC_REMAINDER==18);
    CHECK(strcmp(umc_opcode_text(UMC_REMAINDER),"remainder")==0);
    CHECK(umc_encode(literal,4U,encoded,sizeof encoded,&size,&d)==UMC_OK);
    CHECK(size==sizeof wire && memcmp(encoded,wire,sizeof wire)==0);
    CHECK(umc_decode(wire,sizeof wire,decoded,4U,&count,&d)==UMC_OK && count==4U);
    UmcProgramInfo info; CHECK(umc_inspect(decoded,count,&info,&d)==UMC_OK);
    CHECK(info.constant_count==2U && info.arithmetic_count==1U && info.peak_stack==2U);
    UmcResult r;CHECK(umc_execute(decoded,count,4U,&r,&d)==UMC_OK && r.value==1);
    for(unsigned version=0U;version<7U;++version) {
        if(version==5U)continue;
        memcpy(encoded,wire,sizeof wire);encoded[4]=(uint8_t)version;count=99U;
        CHECK(umc_decode(encoded,sizeof wire,decoded,4U,&count,&d)==UMC_BAD_BYTECODE && count==0U);
    }
    return 0;
}
static int Versions(void)
{
    const char *bodies[]={"return 7%3;","int x=7;return x%3;","return 1?7%3:2;",
        "return 1?2:7%3;","return 7%3?2:3;","int x=7;if(1){x%=3;}return x;"};
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i) {
        char source[256];int length=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction p[UMC_PROGRAM_LIMIT],q[UMC_PROGRAM_LIMIT];uint8_t bytes[UMC_BYTECODE_LIMIT];
        UmcDiagnostic d;size_t n=0U,size=0U,m=0U;
        CHECK(umc_compile(source,(size_t)length,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK);
        CHECK(umc_encode(p,n,bytes,sizeof bytes,&size,&d)==UMC_OK && bytes[4]==5U);
        CHECK(umc_decode(bytes,size,q,UMC_PROGRAM_LIMIT,&m,&d)==UMC_OK && m==n);
    }
    /* Raising the header on an older program is also noncanonical. */
    uint8_t bytes[sizeof wire];memcpy(bytes,wire,sizeof bytes);bytes[26]=(uint8_t)UMC_DIVIDE;
    UmcInstruction p[4];UmcDiagnostic d;size_t n=99U;
    CHECK(umc_decode(bytes,sizeof bytes,p,4U,&n,&d)==UMC_BAD_BYTECODE && n==0U);
    bytes[4]=1U;CHECK(umc_decode(bytes,sizeof bytes,p,4U,&n,&d)==UMC_OK);
    return 0;
}
static int Fuel(void)
{
    UmcTraceStep rows[4];UmcDiagnostic d;UmcResult r={91,73U};size_t used=0U;
    for(size_t fuel=0U;fuel<4U;++fuel) {
        CHECK(umc_execute_trace(literal,4U,fuel,&r,rows,4U,&used,&d)==UMC_FUEL_EXHAUSTED);
        CHECK(r.value==91 && r.instructions_executed==73U && used==fuel+1U);
        CHECK(rows[used-1U].instruction_index==fuel && rows[used-1U].status==UMC_FUEL_EXHAUSTED);
    }
    CHECK(umc_execute_trace(literal,4U,4U,&r,rows,4U,&used,&d)==UMC_OK);
    CHECK(r.value==1 && r.instructions_executed==4U && used==4U);
    CHECK(rows[2].opcode==UMC_REMAINDER && rows[2].top==1 && rows[2].stack_depth==1U);
    return 0;
}
static int Malformed(void)
{
    UmcInstruction p[4];UmcDiagnostic d;UmcResult r={91,73U};
    memcpy(p,literal,sizeof p);p[2].operand=1;
    CHECK(umc_execute(p,4U,0U,&r,&d)==UMC_BAD_BYTECODE && r.value==91);
    memcpy(p,literal,sizeof p);p[1].opcode=UMC_REMAINDER;p[1].operand=0;
    CHECK(umc_validate(p,4U,&d)==UMC_BAD_BYTECODE);
    memcpy(p,literal,sizeof p);p[2].source_offset=UMC_SOURCE_LIMIT+1U;
    CHECK(umc_validate(p,4U,&d)==UMC_BAD_BYTECODE);
/* Opcode 19 is now a supported bitwise AND. Keep rejection coverage on the first unassigned opcode while the version-six suite checks 19 explicitly. */
#if 0
    memcpy(p,literal,sizeof p);p[2].opcode=(UmcOpcode)19;
#endif
/* Opcode 22 is now a checked shift. Use an unassigned byte value for generic refusal and keep exact new identities in the shift suite. The former implementation is retained for owner review. */
#if 0
    memcpy(p,literal,sizeof p);p[2].opcode=(UmcOpcode)22;
#endif
    memcpy(p,literal,sizeof p);p[2].opcode=(UmcOpcode)255;
    CHECK(umc_validate(p,4U,&d)==UMC_BAD_BYTECODE);
    return 0;
}
static int Spans(void)
{
    const char source[]="int main(void){int x=7;x%=3;return x;}";
    char exact[sizeof source-1U];memcpy(exact,source,sizeof exact);
    UmcInstruction p[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t n=0U;
    CHECK(umc_compile(exact,sizeof exact,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK && n==8U);
    const size_t percent=(size_t)(strchr(source,'%')-source);
    for(size_t length=percent;length<=percent+2U;++length) {
        CHECK(umc_compile(exact,length,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR);
        CHECK(n==0U && d.source_offset<=length);
    }
    exact[percent+1U]='\0';
    CHECK(umc_compile(exact,sizeof exact,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR && n==0U);
    const char short_source[]="int main(void){return 7%3;}";
    /* The compiler reports instruction capacity as PROGRAM_TOO_LARGE.
     * Preserve the initial incorrect test expectation; wire output storage has
     * the distinct BUFFER_TOO_SMALL contract checked immediately below. */
#if 0
    CHECK(umc_compile(short_source,sizeof short_source-1U,p,3U,&n,&d)==UMC_BUFFER_TOO_SMALL && n==0U);
#endif
    CHECK(umc_compile(short_source,sizeof short_source-1U,p,3U,&n,&d)==UMC_PROGRAM_TOO_LARGE && n==0U);
    uint8_t bytes[sizeof wire];size_t size=99U;
    CHECK(umc_encode(literal,4U,bytes,sizeof bytes-1U,&size,&d)==UMC_BUFFER_TOO_SMALL && size==0U);
    return 0;
}
static int Native(void)
{
    for(int32_t left=-31;left<=31;++left)for(int32_t right=-31;right<=31;++right) {
        if(right==0)continue;
        char body[128];int length=snprintf(body,sizeof body,"return %" PRId32 "%%%" PRId32 ";",left,right);
        CHECK(length>0 && (size_t)length<sizeof body);
        CHECK(RunBody(body,left%right)==0);
        length=snprintf(body,sizeof body,"int x=%" PRId32 ";x%%=%" PRId32 ";return x;",left,right);
        CHECK(length>0 && (size_t)length<sizeof body);
        int32_t native=left;native%=right;CHECK(RunBody(body,native)==0);
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"values")==0)return Values();
    if(strcmp(argv[1],"faults")==0)return Faults();
    if(strcmp(argv[1],"refusals")==0)return Refusals();
    if(strcmp(argv[1],"wire")==0)return Wire();
    if(strcmp(argv[1],"versions")==0)return Versions();
    if(strcmp(argv[1],"fuel")==0)return Fuel();
    if(strcmp(argv[1],"malformed")==0)return Malformed();
    if(strcmp(argv[1],"spans")==0)return Spans();
    if(strcmp(argv[1],"native")==0)return Native();
    return 2;
}
