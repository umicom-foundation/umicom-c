/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_ascii_characters.c
 *
 * PURPOSE:
 *   Verify ASCII character spelling, bounded escapes, source ownership and existing numeric lowering.
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
#include "umicom_c/source_check.h"
static int value(const char *source, int32_t expected)
{
    UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=0;
    CHECK(umc_compile(source,strlen(source),code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==expected);
    return 0;
}
static int printable(void)
{
    for(unsigned c=32U;c<=126U;++c){
        if(c==39U || c==92U)continue;
        char source[80];int n=snprintf(source,sizeof source,"int main(void){return '%c';}",(int)c);
        CHECK(n>0 && (size_t)n<sizeof source);
        CHECK(value(source,(int32_t)c)==0);
    }
    CHECK(value("int main(void){return -'A';}",-65)==0);
    CHECK(value("int main(void){int c='A';if(c>='A'&&c<='Z'){c+='a'-'A';}return c;}",97)==0);
    CHECK(value("int main(void){return '/'+'*';}",89)==0);
    return 0;
}
static int escapes(void)
{
    static const struct { const char *text;int32_t expected; } cases[]={
        {"'\\a'",7},{"'\\b'",8},{"'\\f'",12},{"'\\n'",10},{"'\\r'",13},
        {"'\\t'",9},{"'\\v'",11},{"'\\\\'",92},{"'\\''",39},{"'\\\"'",34},{"'\\?'",63},
        {"'\\0'",0},{"'\\00'",0},{"'\\000'",0},{"'\\7'",7},{"'\\77'",63},{"'\\177'",127},
        {"'\\x0'",0},{"'\\x7f'",127},{"'\\x7F'",127},{"'\\x000000000000000000000041'",65}
    };
    for(size_t i=0;i<sizeof cases/sizeof cases[0];++i){
        char source[160];int n=snprintf(source,sizeof source,"int main(void){return %s;}",cases[i].text);
        CHECK(n>0 && (size_t)n<sizeof source);CHECK(value(source,cases[i].expected)==0);
    }
    return 0;
}
static int numeric(void)
{
    for(unsigned c=0;c<=127;++c){
        char source[100];int n=snprintf(source,sizeof source,"int main(void){return '\\x%02X';}",c);
        CHECK(n>0 && (size_t)n<sizeof source);CHECK(value(source,(int32_t)c)==0);
        n=snprintf(source,sizeof source,"int main(void){return '\\%03o';}",c);
        CHECK(n>0 && (size_t)n<sizeof source);CHECK(value(source,(int32_t)c)==0);
    }
    CHECK(value("int main(void){return 1'000+'A'-65;}",1000)==0);
    CHECK(value("int main(void){return 0x2'A+'0'-'0';}",42)==0);
    return 0;
}
static int refusals(void)
{
    static const char *const literals[]={"''","'ab'","'''","'\\q'","'\\8'","'\\x'","'\\xg'",
        "'\\u0041'","'\\U00000041'","'\\0000'","'\\08'","L'A'","u'A'","U'A'","u8'A'",
        "'A' 'B'","'A''B'","'A","'\\","'a\\n'","'\n'","'\r'","'\t'","'\\x4'1'"};
    for(size_t i=0;i<sizeof literals/sizeof literals[0];++i){
        char source[128];int n=snprintf(source,sizeof source,"int main(void){return %s;}",literals[i]);
        UmcInstruction code[32];size_t count=73;UmcDiagnostic d;
        CHECK(n>0 && (size_t)n<sizeof source);
        CHECK(umc_compile(source,(size_t)n,code,32,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    }
    for(unsigned c=127;c<=255;++c){
        char source[]="int main(void){return 'A';}";source[23]=(char)c;
        UmcInstruction code[4];size_t count=73;UmcDiagnostic d;
        CHECK(umc_compile(source,sizeof source-1U,code,4,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    }
    return 0;
}
static int ranges(void)
{
    const char *literals[]={"'\\200'","'\\377'","'\\777'","'\\x80'","'\\xFF'","'\\x100'","'\\xFFFFFFFFFFFFFFFFFFFFFFFF'"};
    for(size_t i=0;i<sizeof literals/sizeof literals[0];++i){
        char source[128];int n=snprintf(source,sizeof source,"int main(void){return %s;}",literals[i]);
        UmcInstruction code[4];size_t count=73;UmcDiagnostic d;
        CHECK(n>0 && (size_t)n<sizeof source);
        CHECK(umc_compile(source,(size_t)n,code,4,&count,&d)==UMC_LITERAL_RANGE && count==0U);
        CHECK(d.source_offset==22U);
    }
    return 0;
}
static int spans(void)
{
    const char *sources[]={"int main(void){return 'A';}","int main(void){return '\\'';}",
        "int main(void){return '\\x41';}","int main(void){return '\\101';}"};
    for(size_t i=0;i<sizeof sources/sizeof sources[0];++i){
        char exact[64];size_t length=strlen(sources[i]);memcpy(exact,sources[i],length);
        UmcInstruction code[8];size_t count=73;UmcDiagnostic d;
        CHECK(umc_compile(exact,length,code,8,&count,&d)==UMC_OK);
        for(size_t end=0;end<length;++end)
            CHECK(umc_compile(exact,end,code,8,&count,&d)!=UMC_OK && count==0U);
    }
    char embedded[]="int main(void){return 'A';}";embedded[23]='\0';
    UmcInstruction code[8];size_t count=73;UmcDiagnostic d;
    CHECK(umc_compile(embedded,sizeof embedded-1U,code,8,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    const char *bad="int main(void){\r\nreturn '\\q';}";
    CHECK(umc_compile(bad,strlen(bad),code,8,&count,&d)==UMC_SYNTAX_ERROR);
    CHECK(d.line==2U && d.column==8U);
    return 0;
}
static int wire(void)
{
    const char *source="int main(void){return '*';}";
    UmcInstruction code[4],decoded[4];size_t count=0,used=0,decodedCount=0;
    uint8_t bytes[UMC_BYTECODE_LIMIT];UmcDiagnostic d;UmcResult r;
    CHECK(umc_compile(source,strlen(source),code,4,&count,&d)==UMC_OK && count==2U);
    CHECK(code[0].opcode==UMC_PUSH && code[0].operand==42 && code[0].source_offset==22U);
    CHECK(umc_encode(code,count,bytes,sizeof bytes,&used,&d)==UMC_OK && bytes[4]==1U);
    CHECK(umc_decode(bytes,used,decoded,4,&decodedCount,&d)==UMC_OK && decodedCount==2U);
    CHECK(decoded[0].operand==42 && decoded[0].source_offset==22U);
    CHECK(umc_execute(decoded,decodedCount,2,&r,&d)==UMC_OK && r.value==42);
    r=(UmcResult){71,73};
    CHECK(umc_execute(decoded,decodedCount,1,&r,&d)==UMC_FUEL_EXHAUSTED && r.value==71 && r.instructions_executed==73U);
    CHECK(umc_compile(source,strlen(source),code,1,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    return 0;
}
static int warnings(void)
{
    const char *source="int main(void){int A=1;return 'A';}";UmcSourceReport report;UmcDiagnostic d;
    CHECK(umc_check_source(source,strlen(source),&report,&d)==UMC_OK);
    CHECK(report.warning_count==1U && report.warnings[0].kind==UMC_WARNING_LOCAL_NEVER_READ);
    CHECK(report.warnings[0].source_offset==19U);
    CHECK(value(source,65)==0);
    return 0;
}
static int long_escape(void)
{
    char source[8192];const char *prefix="int main(void){return '\\x";
    size_t length=strlen(prefix);memcpy(source,prefix,length);memset(source+length,'0',4096);length+=4096;
    memcpy(source+length,"2A';}",6);length+=5;
    CHECK(length<sizeof source);CHECK(value(source,42)==0);
    return 0;
}
int main(int argc,char **argv)
{
    CHECK(argc==2 && UMC_ASCII_CHARACTER_PROFILE==1U);
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_ASCII_CHARACTER_LITERALS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_ASCII_CHARACTER_LITERALS));
    if(!strcmp(argv[1],"printable"))return printable();
    if(!strcmp(argv[1],"escapes"))return escapes();
    if(!strcmp(argv[1],"numeric"))return numeric();
    if(!strcmp(argv[1],"refusals"))return refusals();
    if(!strcmp(argv[1],"ranges"))return ranges();
    if(!strcmp(argv[1],"spans"))return spans();
    if(!strcmp(argv[1],"wire"))return wire();
    if(!strcmp(argv[1],"warnings"))return warnings();
    if(!strcmp(argv[1],"long-escape"))return long_escape();
    return 2;
}
