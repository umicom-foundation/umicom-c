/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_if_statement.c
 *
 * PURPOSE:
 *   Verify braced conditional updates, selective faults, combined depth, canonical wire bytes and native-C agreement.
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

static int values(void)
{
    static const struct { const char *body; int32_t value; } cases[] = {
        {"int x=1;if(1){x=42;}return x;",42},
        {"int x=42;if(0){x=1;}return x;",42},
        {"int x=1;if(-7){x=42;}return x;",42},
        {"int x=1;if(0){x=7;}else{x=42;}return x;",42},
        {"int x=1;if(3){x=42;}else{x=7;}return x;",42},
        {"int x=6;if(x<7){x*=7;}else{x=0;}return x;",42},
        {"int x=6;if(x==6){if(x>0){x*=7;}else{x=0;}}else{x=1;}return x;",42},
        {"int x=6;if(0){x=0;}else{if(1){x*=7;}else{x=1;}}return x;",42},
        {"int x=6;if(1){x+=1;x*=6;}return x;",42},
        {"int x=6;if(0&&1/0){x=0;}else{x*=7;}return x;",42},
        {"int x=6;if(1||1/0){x*=7;}return x;",42},
        {"int x=6;if(0?1/0:1){x*=7;}return x;",42},
        {"int x=6;if(0){x=1/0;}else{x*=7;}return x;",42},
        {"int x=6;if(1){x*=7;}else{x=1/0;}return x;",42},
        {"int x=42;if(1){}else{}return x;",42},
        {"if(0){}int x=42;return x;",42},
        {"int x=0;if(1){x=6;}if(x==6){x*=7;}return x;",42},
        {"int if_value=6;int elsewhere=7;if(if_value){if_value*=elsewhere;}return if_value;",42},
        {"int x=6;if/*condition*/(1)/*body*/{x*=7;}else/*body*/{}return x;",42}
    };
    for (size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        char text[512]; int length=snprintf(text,sizeof text,"int main(void){%s}",cases[i].body);
        CHECK(length>0 && (size_t)length<sizeof text);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; UmcResult r; size_t count=0U;
        CHECK(umc_compile(text,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==cases[i].value);
    }
    return 0;
}
static int refusals(void)
{
    static const char *const cases[] = {
        "if(1)return 1;return 0;", "if(1){return 1;}return 0;",
        "int x=0;if(1)x=1;return x;", "int x=0;if(1){x=1;}else x=2;return x;",
        /* Chained branches are now accepted; the former refusal stays for review.
         * Dedicated else-if tests exercise both selection and malformed chains. */
#if 0
        "int x=0;if(1){x=1;}else if(0){x=2;}return x;",
#endif
        "if(1){int x=1;}return 0;", "int x=1;if(0){int y=2;}return x;",
        "int x=1;if(x=2){x=3;}return x;", "int x=1;if(){x=2;}return x;",
        "int x=1;if(x){missing=2;}return x;", "int x=1;if(0){x=missing;}return x;",
        "int x=1;else{x=2;}return x;", "int x=1;if(1){x=2;}else{}else{}return x;",
        "int x=1;if(1){x=2;return x;", "int x=1;if(1){x=2;}return x;else{}",
        "int if=1;return if;", "int else=1;return else;",
        "int x=1;if(1){{x=2;}}return x;", "int x=1;if(1){;}return x;",
        "int x=1;if(1){x=2;}else{}",
        "int x=1;if(0){x=2;}else{int y=3;}return x;"
    };
    for (size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        char text[512]; int length=snprintf(text,sizeof text,"int main(void){%s}",cases[i]);
        CHECK(length>0 && (size_t)length<sizeof text);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; size_t count=99U;
        CHECK(umc_compile(text,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
        CHECK(count==0U && d.source_offset<=(size_t)length && d.line!=0U);
    }
    return 0;
}
static int faults(void)
{
    static const struct { const char *body; const char *marker; } cases[] = {
        {"int x=1;if(1/0){x=42;}return x;","/0"},
        {"int x=1;if(1){x=1/0;}return x;","/0"},
        {"int x=1;if(0){x=42;}else{x=1/0;}return x;","/0"},
        {"int x=2147483647;if(1){x+=1;}return x;","+="}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        char text[256]; int length=snprintf(text,sizeof text,"int main(void){%s}",cases[i].body);
        CHECK(length>0 && (size_t)length<sizeof text);
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcTraceStep rows[UMC_PROGRAM_LIMIT];
        UmcDiagnostic d; UmcResult r={73,91U}; size_t count=0U,used=0U;
        CHECK(umc_compile(text,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute_trace(code,count,count,&r,rows,UMC_PROGRAM_LIMIT,&used,&d)==UMC_ARITHMETIC_ERROR);
        CHECK(r.value==73 && r.instructions_executed==91U);
        CHECK(d.source_offset==(size_t)(strstr(text,cases[i].marker)-text));
        size_t stores=0U;
        for(size_t n=0U;n<used;++n) if(rows[n].opcode==UMC_STORE_LOCAL) ++stores;
        CHECK(stores==1U && rows[used-1U].status==UMC_ARITHMETIC_ERROR);
    }
    return 0;
}
static int spans(void)
{
    const char text[]="int main(void){int x=1;if(0){x=7;}else{x=42;}return x;}";
    char exact[sizeof text-1U]; memcpy(exact,text,sizeof exact);
    UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; size_t count=0U;
    CHECK(umc_compile(exact,sizeof exact,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK && count==11U);
    /* Every proper prefix remains incomplete, including the if/else keywords.
     * This exact span has no trailing NUL and must never be treated as a string. */
    for(size_t length=0U;length<sizeof exact;++length) {
        CHECK(umc_compile(exact,length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR);
        CHECK(count==0U && d.source_offset<=length);
    }
    exact[35]='\0';
    CHECK(umc_compile(exact,sizeof exact,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    const char lines[]="int main(void){\r\nint x=1;\r\nif(x){\r\nx/=0;\r\n}\r\nreturn x;}";
    UmcResult r={73,91U};
    CHECK(umc_compile(lines,sizeof lines-1U,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
    CHECK(umc_execute(code,count,count,&r,&d)==UMC_ARITHMETIC_ERROR);
    umc_locate_diagnostic(lines,sizeof lines-1U,&d);
    CHECK(d.line==4U && d.column==2U);
    return 0;
}
static int nesting(void)
{
    for(size_t nested=UMC_PARSE_DEPTH_LIMIT-1U;nested<=UMC_PARSE_DEPTH_LIMIT;++nested)
        for(size_t extra=0U;extra<=1U;++extra) {
            char text[4096]; size_t length=0U;
            const char start[]="int main(void){int x=0;";
            memcpy(text,start,sizeof start-1U);length=sizeof start-1U;
            for(size_t i=0U;i<nested;++i){memcpy(text+length,"if(1){",6U);length+=6U;}
            memcpy(text+length,"x=",2U);length+=2U;
            if(extra)text[length++]='!';
            text[length++]='0';text[length++]=';';
            for(size_t i=0U;i<nested;++i)text[length++]='}';
            memcpy(text+length,"return x;}",10U);length+=10U;
            UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=99U;
            UmcStatus status=umc_compile(text,length,code,UMC_PROGRAM_LIMIT,&count,&d);
            if(nested+extra<UMC_PARSE_DEPTH_LIMIT) {
                CHECK(status==UMC_OK);
                CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==0);
            } else CHECK(status==UMC_DEPTH_LIMIT && count==0U);
        }
    return 0;
}
static int bounds(void)
{
    for(size_t branches=127U;branches<=128U;++branches) {
        char text[2048];size_t length=0U;
        memcpy(text,"int main(void){",15U);length=15U;
        for(size_t i=0U;i<branches;++i){memcpy(text+length,"if(0){}",7U);length+=7U;}
        memcpy(text+length,"return 42;}",11U);length+=11U;
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=99U;
        UmcStatus status=umc_compile(text,length,code,UMC_PROGRAM_LIMIT,&count,&d);
        if(branches==127U) {
            CHECK(status==UMC_OK && count==256U);
            CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==42);
        } else CHECK(status==UMC_PROGRAM_TOO_LARGE && count==0U);
    }
    const char text[]="int main(void){int x=1;if(0){x=7;}else{x=42;}return x;}";
    UmcInstruction code[11];UmcDiagnostic d;size_t count=99U;
    CHECK(umc_compile(text,sizeof text-1U,code,10U,&count,&d)==UMC_PROGRAM_TOO_LARGE && count==0U);
    CHECK(umc_compile(text,sizeof text-1U,code,11U,&count,&d)==UMC_OK && count==11U);
    const char absent[]="int main(void){if(1){x=2;}return 0;}";
    CHECK(umc_compile(absent,sizeof absent-1U,code,11U,&count,&d)==UMC_SYNTAX_ERROR && count==0U);
    return 0;
}
static int wire_trace(void)
{
    const char text[]="int main(void){int x=1;if(0){x=7;}else{x=42;}return x;}";
    /* Independently enumerated bytes: x declaration19, literal1=21, if23,
     * condition26, first x29/value31, else34, second x39/value41,
     * return45 and final x52. Existing numeric wire opcode identities apply. */
    static const uint8_t expected[]={
        'U','M','C','B',4,0,11,0,
        1,1,0,0,0,21,0,0,0, 15,0,0,0,0,19,0,0,0,
        1,0,0,0,0,26,0,0,0, 17,7,0,0,0,23,0,0,0,
        1,7,0,0,0,31,0,0,0, 15,0,0,0,0,29,0,0,0,
        16,9,0,0,0,34,0,0,0, 1,42,0,0,0,41,0,0,0,
        15,0,0,0,0,39,0,0,0, 14,0,0,0,0,52,0,0,0,
        7,0,0,0,0,45,0,0,0
    };
    UmcInstruction code[11],decoded[11];UmcDiagnostic d;UmcResult r;
    uint8_t bytes[sizeof expected];UmcTraceStep rows[11];size_t count=0U,size=0U,used=0U;
    CHECK(umc_compile(text,sizeof text-1U,code,11U,&count,&d)==UMC_OK && count==11U);
    CHECK(umc_encode(code,count,bytes,sizeof bytes,&size,&d)==UMC_OK && size==sizeof expected);
    CHECK(!memcmp(bytes,expected,sizeof expected));
    CHECK(umc_decode(expected,sizeof expected,decoded,11U,&count,&d)==UMC_OK);
    static const size_t path[]={0U,1U,2U,3U,7U,8U,9U,10U};
    for(size_t fuel=0U;fuel<=8U;++fuel) {
        r.value=73;r.instructions_executed=91U;
        UmcStatus status=umc_execute_trace(decoded,count,fuel,&r,rows,11U,&used,&d);
        if(fuel==8U)CHECK(status==UMC_OK && r.value==42 && r.instructions_executed==8U && used==8U);
        else {
            CHECK(status==UMC_FUEL_EXHAUSTED && used==fuel+1U);
            CHECK(r.value==73 && r.instructions_executed==91U);
            CHECK(rows[fuel].status==UMC_FUEL_EXHAUSTED);
        }
        for(size_t i=0U;i<used;++i)CHECK(rows[i].instruction_index==path[i]);
    }
    CHECK(umc_compiler_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_IF_STATEMENTS);
    CHECK(!(umc_runtime_profile(UMC_PROFILE_FEATURES)&UMC_FEATURE_IF_STATEMENTS));
    return 0;
}
static int native(void)
{
    /* Native C is an independent oracle over small defined integer values.
     * Both outcomes, nested choices and updates are exercised without overflow. */
    for(int left=-9;left<=9;++left)for(int right=-9;right<=9;++right) {
        int expected=left;
        if(left<right){expected+=right;if(expected<0){expected=-expected;}}
        else{expected-=right;}
        char text[256];int length=snprintf(text,sizeof text,
            "int main(void){int x=%d;int y=%d;if(x<y){x+=y;if(x<0){x=-x;}}else{x-=y;}return x;}",left,right);
        CHECK(length>0 && (size_t)length<sizeof text);
        UmcInstruction code[UMC_PROGRAM_LIMIT];UmcDiagnostic d;UmcResult r;size_t count=0U;
        CHECK(umc_compile(text,(size_t)length,code,UMC_PROGRAM_LIMIT,&count,&d)==UMC_OK);
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK && r.value==expected);
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"values"))return values();
    if(!strcmp(argv[1],"refusals"))return refusals();
    if(!strcmp(argv[1],"faults"))return faults();
    if(!strcmp(argv[1],"spans"))return spans();
    if(!strcmp(argv[1],"nesting"))return nesting();
    if(!strcmp(argv[1],"bounds"))return bounds();
    if(!strcmp(argv[1],"wire-trace"))return wire_trace();
    if(!strcmp(argv[1],"native"))return native();
    return 2;
}
