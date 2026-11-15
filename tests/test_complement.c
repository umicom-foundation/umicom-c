/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_complement.c
 *
 * PURPOSE:
 *   Verify signed complement lowering, endpoints, spans, wire identity, trace cost and native-C agreement.
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
#include "umicom_c/profile.h"
#include <inttypes.h>
#include <limits.h>

static int RunBody(const char *body,int32_t expected,unsigned version)
{
    char source[512];int length=snprintf(source,sizeof source,"int main(void){%s}",body);
    CHECK(length>0 && (size_t)length<sizeof source);
    UmcInstruction p[UMC_PROGRAM_LIMIT],q[UMC_PROGRAM_LIMIT];
    uint8_t wire[UMC_BYTECODE_LIMIT];UmcDiagnostic d;UmcResult r;
    size_t n=0U,m=0U,bytes=0U;
    CHECK(umc_compile(source,(size_t)length,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK);
    CHECK(umc_execute(p,n,n,&r,&d)==UMC_OK && r.value==expected);
    CHECK(umc_encode(p,n,wire,sizeof wire,&bytes,&d)==UMC_OK && wire[4]==version);
    CHECK(umc_decode(wire,bytes,q,UMC_PROGRAM_LIMIT,&m,&d)==UMC_OK && m==n);
    CHECK(umc_execute(q,m,m,&r,&d)==UMC_OK && r.value==expected);
    for(size_t i=0U;i<n;++i) {
        CHECK(p[i].opcode==q[i].opcode && p[i].operand==q[i].operand);
        CHECK(p[i].source_offset==q[i].source_offset);
    }
    return 0;
}
static int Values(void)
{
    static const struct {const char *body;int32_t value;unsigned version;} cases[]={
        {"return ~0;",-1,1},{"return ~1;",-2,1},{"return ~-1;",0,1},
        {"return ~2147483647;",INT32_MIN,1},{"return ~(-2147483647-1);",INT32_MAX,1},
        {"return ~~(-2147483647-1);",INT32_MIN,1},{"return ~~~0;",-1,1},
        {"return ~0+2;",1,1},{"return ~(0+2);",-3,1},{"return 2*~1;",-4,1},
        {"return ~+41;",-42,1},{"return -~41;",42,1},{"return ~!0;",-2,2},
        {"return !~(-1);",1,2},{"return ~0== -1;",1,2},
        {"return ~0x2a;",-43,1},{"return ~0b101010;",-43,1},
        {"return ~/*one*/41;",-42,1},{"return ~ //line\n41;",-42,1},
        {"int x=41;return ~x;",-42,3},{"int x=1;x+=~0;return x;",0,3},
        {"return ~(1?41:1/0);",-42,4},{"return ~(0?1/0:41);",-42,4},
        {"return ~(0&&1/0);",-1,4},{"return ~(1||1/0);",-2,4},
        {"return 0&&~(1/0);",0,4},{"return 1?42:~(1/0);",42,4},
        {"int x=0;if(0){x=~(1/0);}else if(1){x=~-43;}return x;",42,4},
        {"return ~(7%3);",-2,5},{"return ~41%5;",-2,5}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i)
        CHECK(RunBody(cases[i].body,cases[i].value,cases[i].version)==0);
    return 0;
}
static int Trace(void)
{
    const char source[]="int main(void){return ~41;}";
    UmcInstruction p[4];UmcDiagnostic d;size_t n=0U,used=0U;
    const size_t at=(size_t)(strchr(source,'~')-source);
    CHECK(umc_compile(source,sizeof source-1U,p,4U,&n,&d)==UMC_OK && n==4U);
    CHECK(p[0].opcode==UMC_PUSH && p[0].operand==-1 && p[0].source_offset==at);
    CHECK(p[1].opcode==UMC_PUSH && p[1].operand==41 && p[1].source_offset==at+1U);
    CHECK(p[2].opcode==UMC_SUBTRACT && p[2].operand==0 && p[2].source_offset==at);
    CHECK(p[3].opcode==UMC_RETURN);
    UmcProgramInfo info;
    CHECK(umc_inspect(p,n,&info,&d)==UMC_OK);
    CHECK(info.peak_stack==2U && info.constant_count==2U && info.arithmetic_count==1U);
    UmcTraceStep rows[4];UmcResult r={91,73U};
    for(size_t fuel=0U;fuel<4U;++fuel) {
        CHECK(umc_execute_trace(p,n,fuel,&r,rows,4U,&used,&d)==UMC_FUEL_EXHAUSTED);
        CHECK(r.value==91 && r.instructions_executed==73U && used==fuel+1U);
        CHECK(rows[used-1U].instruction_index==fuel && d.source_offset==p[fuel].source_offset);
    }
    CHECK(umc_execute_trace(p,n,4U,&r,rows,4U,&used,&d)==UMC_OK);
    CHECK(r.value==-42 && r.instructions_executed==4U && used==4U);
    CHECK(rows[0].top==-1 && rows[0].stack_depth==1U);
    CHECK(rows[1].top==41 && rows[1].stack_depth==2U);
    CHECK(rows[2].top==-42 && rows[2].stack_depth==1U);
    return 0;
}
static int Faults(void)
{
    static const struct {const char *body;char op;UmcOpcode opcode;} cases[]={
        {"return ~(1/0);",'/',UMC_DIVIDE},
        {"return ~(2147483647+1);",'+',UMC_ADD},
        {"return ~((-2147483647-1)%-1);",'%',UMC_REMAINDER}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        char source[256];int length=snprintf(source,sizeof source,"int main(void){%s}",cases[i].body);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction p[32];UmcTraceStep rows[32];UmcDiagnostic d;
        UmcResult r={91,73U};size_t n=0U,used=0U;
        CHECK(umc_compile(source,(size_t)length,p,32U,&n,&d)==UMC_OK);
        CHECK(umc_execute_trace(p,n,n,&r,rows,32U,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==91 && r.instructions_executed==73U && used>0U);
        CHECK(rows[used-1U].opcode==cases[i].opcode);
        CHECK(d.source_offset==(size_t)(strchr(source,cases[i].op)-source));
    }
    return 0;
}
/* The binary bitwise family now accepts former complete expressions; retain missing-operand and shift refusals, with positive cases in the bitwise suite. The previous implementation is retained for engineering review. */
#if 0
static int Refusals(void)
{
    const char *bodies[]={"return ~;","return ~=0;","return 1~0;","return ~missing;",
        "int x=1;x~=1;return x;","return ~1u;","return ~1.0;",
        "return ~0&1;","return ~0|1;","return ~0^1;","return ~0<<1;"};
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i) {
        char source[256];int length=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction p[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t n=99U;
        CHECK(umc_compile(source,(size_t)length,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR && n==0U);
    }
    return 0;
}
#endif
static int Refusals(void)
{
    const char *bodies[]={"return ~;","return ~=0;","return 1~0;","return ~missing;",
        "int x=1;x~=1;return x;","return ~1u;","return ~1.0;",
        "return ~0&;","return ~0|;","return ~0^;","return ~0<<1;"};
    for(size_t i=0U;i<sizeof bodies/sizeof bodies[0];++i) {
        char source[256];int length=snprintf(source,sizeof source,"int main(void){%s}",bodies[i]);
        CHECK(length>0 && (size_t)length<sizeof source);
        UmcInstruction p[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t n=99U;
        CHECK(umc_compile(source,(size_t)length,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR && n==0U);
    }
    return 0;
}
static int Spans(void)
{
    const char source[]="int main(void){return ~41;}";
    char span[sizeof source-1U];memcpy(span,source,sizeof span);
    UmcInstruction p[8];UmcDiagnostic d;size_t n=0U;
    CHECK(umc_compile(span,sizeof span,p,8U,&n,&d)==UMC_OK && n==4U);
    const size_t end=(size_t)(strchr(source,'~')-source)+1U;
    CHECK(umc_compile(span,end,p,8U,&n,&d)==UMC_SYNTAX_ERROR);
    CHECK(n==0U && d.source_offset==end);
    span[end]='\0';
    CHECK(umc_compile(span,sizeof span,p,8U,&n,&d)==UMC_SYNTAX_ERROR && n==0U);
    return 0;
}
static int Capacity(void)
{
    const char source[]="int main(void){return ~41;}";
    UmcInstruction p[4];UmcDiagnostic d;size_t n=99U,used=99U;
    CHECK(umc_compile(source,sizeof source-1U,p,3U,&n,&d)==UMC_PROGRAM_TOO_LARGE && n==0U);
    CHECK(umc_compile(source,sizeof source-1U,p,4U,&n,&d)==UMC_OK);
    UmcResult r={91,73U};UmcTraceStep rows[3];
    CHECK(umc_execute_trace(p,n,n,&r,rows,3U,&used,&d)==UMC_BUFFER_TOO_SMALL && used==0U);
    CHECK(r.value==91 && r.instructions_executed==73U);
    uint8_t wire[44];size_t bytes=99U;
    CHECK(umc_encode(p,n,wire,sizeof wire-1U,&bytes,&d)==UMC_BUFFER_TOO_SMALL && bytes==0U);
    return 0;
}
static int Depth(void)
{
    char source[160]="int main(void){return ";
    const size_t prefix=strlen(source);
    memset(source+prefix,'~',UMC_PARSE_DEPTH_LIMIT-1U);
    memcpy(source+prefix+UMC_PARSE_DEPTH_LIMIT-1U,"0;}",3U);
    UmcInstruction p[UMC_PROGRAM_LIMIT];UmcDiagnostic d;size_t n=0U;
    CHECK(umc_compile(source,prefix+UMC_PARSE_DEPTH_LIMIT+2U,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK);
    CHECK(n==2U*UMC_PARSE_DEPTH_LIMIT);
    UmcProgramInfo info;UmcResult r;
    CHECK(umc_inspect(p,n,&info,&d)==UMC_OK && info.peak_stack==UMC_STACK_LIMIT);
    CHECK(umc_execute(p,n,n,&r,&d)==UMC_OK && r.value==-1);
    memset(source+prefix,'~',UMC_PARSE_DEPTH_LIMIT);
    memcpy(source+prefix+UMC_PARSE_DEPTH_LIMIT,"0;}",3U);
    CHECK(umc_compile(source,prefix+UMC_PARSE_DEPTH_LIMIT+3U,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_DEPTH_LIMIT && n==0U);
    return 0;
}
static int Wire(void)
{
    /* Independent format-1 fixture: no compiler call builds this oracle. */
    static const uint8_t bytes[]={
        'U','M','C','B',1,0,4,0,
        1,255,255,255,255,10,0,0,0,
        1,0,0,0,128,20,0,0,0,
        3,0,0,0,0,10,0,0,0,
        7,0,0,0,0,30,0,0,0
    };
    UmcInstruction p[4];UmcDiagnostic d;size_t n=0U,size=0U;
    uint8_t encoded[sizeof bytes];
    CHECK(umc_decode(bytes,sizeof bytes,p,4U,&n,&d)==UMC_OK && n==4U);
    UmcResult r;CHECK(umc_execute(p,n,n,&r,&d)==UMC_OK && r.value==INT32_MAX);
    CHECK(umc_encode(p,n,encoded,sizeof encoded,&size,&d)==UMC_OK);
    CHECK(size==sizeof bytes && memcmp(encoded,bytes,sizeof bytes)==0);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_BITWISE_COMPLEMENT));
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_BITWISE_COMPLEMENT);
    /* Preserve the format-5 mask; the runtime now also admits format 6. */
#if 0
    CHECK(umc_runtime_profile(UMC_PROFILE_WIRE_VERSIONS)==62U);
#endif
    CHECK(umc_runtime_profile(UMC_PROFILE_WIRE_VERSIONS)==126U);
    return 0;
}
static int Native(void)
{
    for(int32_t x=-257;x<=257;++x) {
        char body[128];int length=snprintf(body,sizeof body,"return ~%" PRId32 ";",x);
        CHECK(length>0 && (size_t)length<sizeof body);
        CHECK(RunBody(body,(int32_t)~x,1U)==0);
    }
    CHECK(RunBody("return ~2147483647;",(int32_t)~INT32_MAX,1U)==0);
    CHECK(RunBody("return ~(-2147483647-1);",(int32_t)~INT32_MIN,1U)==0);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"values")==0)return Values();
    if(strcmp(argv[1],"trace")==0)return Trace();
    if(strcmp(argv[1],"faults")==0)return Faults();
    if(strcmp(argv[1],"refusals")==0)return Refusals();
    if(strcmp(argv[1],"spans")==0)return Spans();
    if(strcmp(argv[1],"capacity")==0)return Capacity();
    if(strcmp(argv[1],"depth")==0)return Depth();
    if(strcmp(argv[1],"wire")==0)return Wire();
    if(strcmp(argv[1],"native")==0)return Native();
    return 2;
}
