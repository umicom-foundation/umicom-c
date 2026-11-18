/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_digit_separators.c
 *
 * PURPOSE:
 *   Verify separated decimal, binary and hexadecimal spelling retains exact values, error spans and wire operations.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


#include "test.h"
#include <limits.h>
#include "umicom_c/profile.h"

static int Value(const char *literal,int32_t expected)
{
    char source[512];
    int length=snprintf(source,sizeof(source),"int main(void){return %s;}",literal);
    CHECK(length>0&&(size_t)length<sizeof(source));
    UmcInstruction instructions[64],decoded[64];size_t count=0U,restored=0U,bytes=0U;
    UmcResult result;UmcDiagnostic diagnostic;uint8_t wire[1024];
    CHECK(umc_compile(source,(size_t)length,instructions,64U,&count,&diagnostic)==UMC_OK);
    CHECK(umc_execute(instructions,count,count,&result,&diagnostic)==UMC_OK&&result.value==expected);
    CHECK(umc_encode(instructions,count,wire,sizeof(wire),&bytes,&diagnostic)==UMC_OK);
    CHECK(umc_decode(wire,bytes,decoded,64U,&restored,&diagnostic)==UMC_OK);
    CHECK(umc_execute(decoded,restored,restored,&result,&diagnostic)==UMC_OK&&result.value==expected);
    return 0;
}
static int Values(void)
{
    const struct {const char *text;int32_t value;} cases[]={
        {"1'000",1000},{"0'0'1'0",10},{"2'147'483'647",INT32_MAX},
        {"-2'147'483'647-1",INT32_MIN},{"0xCA'FE",51966},{"0X7FFF'FFFF",INT32_MAX},
        {"0x0'0'0'A",10},{"0b1010'1010",170},{"0B0'1'0",2},
        {"0b111'1111'1111'1111'1111'1111'1111'1111",INT32_MAX},
        {"1'0 + 0xA'A * 0b1'0",350},{"1'0<2'0 ? 0x2'A : 0",42}
    };
    for(size_t i=0U;i<sizeof(cases)/sizeof(cases[0]);++i)CHECK(Value(cases[i].text,cases[i].value)==0);
    return 0;
}
static int Refusals(void)
{
    const char *cases[]={"1'","1''0","1'a","0x'1","0b'1","0x1'","0b1'",
        "0x1'G","0b1'2","1' 0","1'/*x*/0","1'\n0","1'\r\n0","'1"};
    const char prefix[]="int main(void){return ";
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        char source[128];int length=snprintf(source,sizeof(source),"%s%s;}",prefix,cases[i]);
        CHECK(length>0&&(size_t)length<sizeof(source));
        UmcInstruction program[8];UmcDiagnostic diagnostic;size_t count=777U;
        CHECK(umc_compile(source,(size_t)length,program,8U,&count,&diagnostic)==UMC_SYNTAX_ERROR);
        CHECK(count==0U);
        const char *quote=strchr(cases[i],'\'');CHECK(quote!=NULL);
        CHECK(diagnostic.source_offset==sizeof(prefix)-1U+(size_t)(quote-cases[i]));
    }
    return 0;
}
static int Overflow(void)
{
    const char *cases[]={"2'147'483'648","0x8'000'0000","0b1'0000000000000000000000000000000"};
    const char prefix[]="int main(void){return ";
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        char source[128];int length=snprintf(source,sizeof(source),"%s%s;}",prefix,cases[i]);
        CHECK(length>0&&(size_t)length<sizeof(source));
        UmcInstruction program[8];UmcDiagnostic diagnostic;size_t count=777U;
        CHECK(umc_compile(source,(size_t)length,program,8U,&count,&diagnostic)==UMC_LITERAL_RANGE);
        CHECK(count==0U&&diagnostic.source_offset==sizeof(prefix)-1U);
    }
    return 0;
}
static int Spans(void)
{
    const char fixture[]="int main(void){return 0x2'A+1'0;}";
    char source[sizeof(fixture)-1U];memcpy(source,fixture,sizeof(source));
    UmcInstruction program[8];UmcDiagnostic diagnostic;size_t count;
    for(size_t length=0;length<sizeof(source);++length){
        CHECK(umc_compile(source,length,program,8U,&count,&diagnostic)!=UMC_OK);
        CHECK(count==0U&&diagnostic.source_offset<=length);
    }
    CHECK(umc_compile(source,sizeof(source),program,8U,&count,&diagnostic)==UMC_OK);
    UmcResult result;CHECK(umc_execute(program,count,count,&result,&diagnostic)==UMC_OK&&result.value==52);
    char invalid[]="int main(void){return 1'0;}";
    char *quote=strchr(invalid,'\'');CHECK(quote!=NULL);quote[1]=0;
    CHECK(umc_compile(invalid,sizeof(invalid)-1U,program,8U,&count,&diagnostic)==UMC_SYNTAX_ERROR);
    CHECK(count==0U&&diagnostic.source_offset==(size_t)(quote-invalid));
    return 0;
}
static int Wire(void)
{
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES) & UMC_FEATURE_DIGIT_SEPARATORS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES) & UMC_FEATURE_DIGIT_SEPARATORS));
    /* Literal and return offsets are independent of digits at the end of this
     * expression. This fixed vector proves separators need no new opcode. */
    const uint8_t expected[]={'U','M','C','B',1,0,2,0,1,42,0,0,0,22,0,0,0,7,0,0,0,0,15,0,0,0};
    const char *cases[]={"int main(void){return 4'2;}","int main(void){return 0x2'A;}","int main(void){return 0b10'1010;}"};
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        UmcInstruction program[8];size_t count=0,size=0;uint8_t bytes[128];
        CHECK(umc_compile(cases[i],strlen(cases[i]),program,8U,&count,NULL)==UMC_OK);
        CHECK(umc_encode(program,count,bytes,sizeof(bytes),&size,NULL)==UMC_OK);
        CHECK(size==sizeof(expected)&&memcmp(bytes,expected,size)==0);
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"values")==0)return Values();
    if(strcmp(argv[1],"refusals")==0)return Refusals();
    if(strcmp(argv[1],"overflow")==0)return Overflow();
    if(strcmp(argv[1],"spans")==0)return Spans();
    if(strcmp(argv[1],"wire")==0)return Wire();
    return 2;
}
