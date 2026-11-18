/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_diagnostic_excerpt.c
 *
 * PURPOSE:
 *   Check source windows, escaped bytes, CRLF locations and failure-preserving diagnostic output.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
#include "umicom_c/diagnostic_excerpt.h"
#include <stdint.h>

static int basic(void)
{
    const char source[] = {'a','b','c'}; /* Deliberately not NUL terminated. */
    UmcDiagnosticExcerpt out;
    CHECK(umc_diagnostic_excerpt(source, sizeof source, 1, &out) == UMC_OK);
    CHECK(strcmp(out.text, "abc") == 0 && out.caret_column == 1);
    CHECK(out.line == 1 && out.column == 2);
    CHECK(out.source_begin == 0 && out.source_end == 3);
    CHECK(!out.clipped_left && !out.clipped_right);
    CHECK(memcmp(source, "abc", 3) == 0);
    return 0;
}
static int empty(void)
{
    UmcDiagnosticExcerpt out;
    CHECK(umc_diagnostic_excerpt("", 0, 0, &out) == UMC_OK);
    CHECK(out.text[0] == 0 && out.caret_column == 0);
    CHECK(out.line == 1 && out.column == 1);
    CHECK(umc_diagnostic_excerpt("abc", 3, 3, &out) == UMC_OK);
    CHECK(strcmp(out.text, "abc") == 0 && out.caret_column == 3 && out.column == 4);
    CHECK(umc_diagnostic_excerpt("abc\n", 4, 4, &out) == UMC_OK);
    CHECK(out.text[0] == 0 && out.line == 2 && out.column == 1);
    return 0;
}
static int newlines(void)
{
    const char source[] = "a\r\nb\rc\nd";
    const size_t lines[] = {1,1,2,2,2,3,3,4,4};
    const size_t columns[] = {1,2,1,1,2,1,2,1,2};
    const char *texts[] = {"a","a","b","b","b","c","c","d","d"};
    for (size_t i=0; i<sizeof source; ++i) {
        UmcDiagnosticExcerpt out;
        CHECK(umc_diagnostic_excerpt(source, sizeof source-1U, i, &out)==UMC_OK);
        CHECK(out.line==lines[i] && out.column==columns[i]);
        CHECK(strcmp(out.text,texts[i])==0 && out.caret_column==columns[i]-1U);
    }
    CHECK(umc_diagnostic_excerpt("\r\n",2,1,&(UmcDiagnosticExcerpt){0})==UMC_OK);
    return 0;
}
static int escapes(void)
{
    const char source[] = {'A','\t','\\',0,27,(char)0xC3,(char)0xA9,'Z'};
    UmcDiagnosticExcerpt out;
    CHECK(umc_diagnostic_excerpt(source,sizeof source,7,&out)==UMC_OK);
    CHECK(strcmp(out.text,"A\\t\\\\\\x00\\x1B\\xC3\\xA9Z")==0);
    CHECK(out.caret_column==21 && out.column==8);
    return 0;
}
static int all_bytes(void)
{
    const char hex[]="0123456789ABCDEF";
    for (unsigned value=0;value<256;++value) {
        if (value==10 || value==13) continue; /* They delimit lines. */
        char source[]={'x',(char)(unsigned char)value,'y'};
        char expected[7]={'x',0}; size_t used=1;
        if(value==92) { expected[used++]='\\';expected[used++]='\\'; }
        else if(value==9) { expected[used++]='\\';expected[used++]='t'; }
        else if(value>=32 && value<=126) expected[used++]=(char)value;
        else {expected[used++]='\\';expected[used++]='x';expected[used++]=hex[value/16];expected[used++]=hex[value%16];}
        const size_t caret=used;expected[used++]='y';expected[used]=0;
        UmcDiagnosticExcerpt out;
        CHECK(umc_diagnostic_excerpt(source,3,2,&out)==UMC_OK);
        CHECK(strcmp(out.text,expected)==0 && out.caret_column==caret);
        CHECK(out.column==3);
    }
    return 0;
}
static int window(void)
{
    char source[200];memset(source,'a',sizeof source);
    for(size_t offset=0;offset<=sizeof source;++offset) {
        UmcDiagnosticExcerpt out;
        CHECK(umc_diagnostic_excerpt(source,sizeof source,offset,&out)==UMC_OK);
        CHECK(out.source_end-out.source_begin==80);
        CHECK(out.source_begin<=offset && out.source_end>=offset);
        CHECK(out.clipped_left==(out.source_begin>0));
        CHECK(out.clipped_right==(out.source_end<sizeof source));
        CHECK(out.caret_column==offset-out.source_begin+(out.clipped_left?3U:0U));
        CHECK(strlen(out.text)==80U+(out.clipped_left?3U:0U)+(out.clipped_right?3U:0U));
    }
    return 0;
}
static int maximum(void)
{
    static char source[UMC_SOURCE_LIMIT];memset(source,0xFF,sizeof source);
    const size_t offsets[]={0,40,80,UMC_SOURCE_LIMIT/2,UMC_SOURCE_LIMIT};
    for(size_t i=0;i<sizeof offsets/sizeof offsets[0];++i) {
        UmcDiagnosticExcerpt out;
        CHECK(umc_diagnostic_excerpt(source,sizeof source,offsets[i],&out)==UMC_OK);
        CHECK(strlen(out.text)<sizeof out.text && out.caret_column<=strlen(out.text));
        CHECK(out.source_end-out.source_begin==80);
        CHECK(out.column==offsets[i]+1U);
        for(size_t j=0;j<strlen(out.text);++j) CHECK(out.text[j]>=32 && out.text[j]<=126);
    }
    return 0;
}
static int refusal(void)
{
    UmcDiagnosticExcerpt out,saved;memset(&out,0x5A,sizeof out);memcpy(&saved,&out,sizeof out);
    CHECK(umc_diagnostic_excerpt(NULL,0,0,&out)==UMC_INVALID_ARGUMENT);
    CHECK(memcmp(&saved,&out,sizeof out)==0);
    CHECK(umc_diagnostic_excerpt("",UMC_SOURCE_LIMIT+1U,0,&out)==UMC_INVALID_ARGUMENT);
    CHECK(memcmp(&saved,&out,sizeof out)==0);
    CHECK(umc_diagnostic_excerpt("abc",3,4,&out)==UMC_INVALID_ARGUMENT);
    CHECK(memcmp(&saved,&out,sizeof out)==0);
    CHECK(umc_diagnostic_excerpt("abc",3,SIZE_MAX,&out)==UMC_INVALID_ARGUMENT);
    CHECK(memcmp(&saved,&out,sizeof out)==0);
    CHECK(umc_diagnostic_excerpt("abc",3,0,NULL)==UMC_INVALID_ARGUMENT);
    return 0;
}
static int surrounding_lines(void)
{
    char source[208];memcpy(source,"before\n",7);memset(source+7,'b',200);source[207]='\n';
    UmcDiagnosticExcerpt out;
    CHECK(umc_diagnostic_excerpt(source,sizeof source,107,&out)==UMC_OK);
    CHECK(out.line==2 && out.column==101);
    CHECK(out.source_begin==67 && out.source_end==147 && out.caret_column==43);
    CHECK(umc_diagnostic_excerpt(source,sizeof source,208,&out)==UMC_OK);
    CHECK(out.line==3 && out.text[0]==0);
    return 0;
}
int main(int argc,char **argv)
{
    CHECK(argc==2);
    if(!strcmp(argv[1],"basic")) return basic();
    if(!strcmp(argv[1],"empty")) return empty();
    if(!strcmp(argv[1],"newlines")) return newlines();
    if(!strcmp(argv[1],"escapes")) return escapes();
    if(!strcmp(argv[1],"all-bytes")) return all_bytes();
    if(!strcmp(argv[1],"window")) return window();
    if(!strcmp(argv[1],"maximum")) return maximum();
    if(!strcmp(argv[1],"refusal")) return refusal();
    if(!strcmp(argv[1],"surrounding-lines")) return surrounding_lines();
    return 2;
}
