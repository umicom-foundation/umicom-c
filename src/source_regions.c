/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/source_regions.c
 *
 * PURPOSE:
 *   Recognize bounded explicit language envelopes with stable original source offsets.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "umicom_c/source_regions.h"
#include "internal.h"
static int space(char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
static int same(const char *s,size_t n,const char *name)
{
    size_t i=0;while(i<n && name[i] && s[i]==name[i])++i;
    return i==n && name[i]=='\0';
}
/* Retain the earlier four-language name table; D is appended without renumbering its predecessors. */
#if 0
const char *umc_source_language_name(UmcSourceLanguage language)
{
    switch(language){case UMC_LANGUAGE_UMICOM:return "umicom";case UMC_LANGUAGE_C23:return "c23";
        case UMC_LANGUAGE_BASIC:return "basic";case UMC_LANGUAGE_JAVA:return "java";default:return "unknown";}
}
#endif
const char *umc_source_language_name(UmcSourceLanguage language)
{
    switch(language){case UMC_LANGUAGE_UMICOM:return "umicom";case UMC_LANGUAGE_C23:return "c23";
        case UMC_LANGUAGE_BASIC:return "basic";case UMC_LANGUAGE_JAVA:return "java";case UMC_LANGUAGE_D:return "d";default:return "unknown";}
}
static UmcStatus fail_at(const char *source,size_t length,UmcDiagnostic *d,size_t offset,const char *message)
{
    umc_fail(d,UMC_SYNTAX_ERROR,offset);
    if(d){d->message=message;umc_locate_diagnostic(source,length,d);}
    return UMC_SYNTAX_ERROR;
}
static int add(UmcSourceRegion *regions,size_t capacity,size_t *used,UmcSourceLanguage lang,size_t start,size_t end,size_t tag)
{
    if(*used>=capacity || *used>=UMC_REGION_LIMIT)return 0;
    UmcSourceRegion *r=&regions[(*used)++];r->language=lang;r->offset=start;r->length=end-start;r->tag_offset=tag;
    return 1;
}
/* This scanner deliberately recognizes only a documented comment boundary,
 * not arbitrary BASIC syntax. It never reads beyond the borrowed source span. */
static int basic_rem_at(const char *source, size_t length, size_t offset)
{
    if (length - offset < 3U) return 0;
    if ((source[offset] != 'R' && source[offset] != 'r') ||
        (source[offset + 1U] != 'E' && source[offset + 1U] != 'e') ||
        (source[offset + 2U] != 'M' && source[offset + 2U] != 'm')) return 0;
    return length - offset == 3U || space(source[offset + 3U]);
}
/* Source-region discovery now protects explicit BASIC REM statement comments; the previous scanner is retained for review and all existing offset/limit contracts remain. The former implementation is retained for owner review. */
#if 0
UmcStatus umc_source_regions(const char *source,size_t length,UmcSourceRegion *regions,size_t capacity,size_t *count,UmcDiagnostic *d)
{
    if(!count)return umc_fail(d,UMC_INVALID_ARGUMENT,0);
    *count=0;
    if(!source || !regions)return umc_fail(d,UMC_INVALID_ARGUMENT,0);
    if(length>UMC_SOURCE_LIMIT)return umc_fail(d,UMC_SOURCE_TOO_LARGE,0);
    for(size_t n=0;n<length;++n)if(source[n]=='\0')return fail_at(source,length,d,n,"NUL is not a source character");
    size_t used=0,start=0,tag=0,i=0,quote_start=0;
    int tagged=0,state=0; /* 1 line comment, 2 block, 3 string, 4 character. */
    UmcSourceLanguage lang=UMC_LANGUAGE_UMICOM;
    while(i<length) {
        char c=source[i],next=i+1<length?source[i+1]:'\0';
        if(state==1){if(c=='\r'||c=='\n')state=0;++i;continue;}
        if(state==2){if(c=='*'&&next=='/'){state=0;i+=2;}else ++i;continue;}
        if(state==3||state==4){
            const char quote=state==3?'"':'\'';
            if(lang==UMC_LANGUAGE_BASIC && c=='"' && next=='"'){i+=2;continue;}
            if(lang!=UMC_LANGUAGE_BASIC && c=='\\'){i+=next?2U:1U;continue;}
            if(c==quote)state=0;
            ++i;continue;
        }
        if(c=='<'&&next=='?') {
            if(tagged)return fail_at(source,length,d,i,"Language tags cannot nest");
            if(i>start && !add(regions,capacity,&used,lang,start,i,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,i);
            size_t name=i+2,j=name;while(j<length && source[j]>='a'&&source[j]<='z')++j;
            if(j<length && source[j]>='0'&&source[j]<='9')while(j<length && source[j]>='0'&&source[j]<='9')++j;
            if(j==length || !space(source[j]))return fail_at(source,length,d,i,"A language tag requires trailing whitespace");
            if(same(source+name,j-name,"umicom"))lang=UMC_LANGUAGE_UMICOM;
            else if(same(source+name,j-name,"c23"))lang=UMC_LANGUAGE_C23;
            else if(same(source+name,j-name,"basic"))lang=UMC_LANGUAGE_BASIC;
            else if(same(source+name,j-name,"java"))lang=UMC_LANGUAGE_JAVA;
            else return fail_at(source,length,d,i,"Unknown language tag");
            tag=i;start=j;i=j;tagged=1;continue;
        }
        if(c=='?'&&next=='>') {
            if(!tagged)return fail_at(source,length,d,i,"Closing tag has no opening tag");
            if(!add(regions,capacity,&used,lang,start,i,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,i);
            tagged=0;lang=UMC_LANGUAGE_UMICOM;i+=2;start=i;tag=i;continue;
        }
        if(lang==UMC_LANGUAGE_BASIC && c=='\''){state=1;++i;continue;}
        if(lang!=UMC_LANGUAGE_BASIC && c=='/'&&next=='/'){state=1;i+=2;continue;}
        if(lang!=UMC_LANGUAGE_BASIC && c=='/'&&next=='*'){state=2;quote_start=i;i+=2;continue;}
        if(c=='"' || (lang!=UMC_LANGUAGE_BASIC && c=='\'')){state=c=='"'?3:4;quote_start=i;}
        ++i;
    }
    if(state==2||state==3||state==4)return fail_at(source,length,d,quote_start,"Unterminated quoted text or comment");
    if(tagged)return fail_at(source,length,d,tag,"Language tag is missing its closing delimiter");
    if(start<length && !add(regions,capacity,&used,lang,start,length,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,start);
    *count=used;return umc_fail(d,UMC_OK,0);
}

#endif
/* The preceding REM-capable scanner lacked BASIC block-comment state.
 * Its complete implementation is retained below for comparison; the active
 * scanner adds bounded iterative nesting while preserving the existing API. */
#if 0
UmcStatus umc_source_regions(const char *source,size_t length,UmcSourceRegion *regions,size_t capacity,size_t *count,UmcDiagnostic *d)
{
    if(!count)return umc_fail(d,UMC_INVALID_ARGUMENT,0);
    *count=0;
    if(!source || !regions)return umc_fail(d,UMC_INVALID_ARGUMENT,0);
    if(length>UMC_SOURCE_LIMIT)return umc_fail(d,UMC_SOURCE_TOO_LARGE,0);
    for(size_t n=0;n<length;++n)if(source[n]=='\0')return fail_at(source,length,d,n,"NUL is not a source character");
    size_t used=0,start=0,tag=0,i=0,quote_start=0;
    int basic_statement_start=1;
    int tagged=0,state=0; /* 1 line comment, 2 block, 3 string, 4 character. */
    UmcSourceLanguage lang=UMC_LANGUAGE_UMICOM;
    while(i<length) {
        char c=source[i],next=i+1<length?source[i+1]:'\0';
        if(state==1){if(c=='\r'||c=='\n'){state=0;basic_statement_start=1;}++i;continue;}
        if(state==2){if(c=='*'&&next=='/'){state=0;i+=2;}else ++i;continue;}
        if(state==3||state==4){
            const char quote=state==3?'"':'\'';
            if(lang==UMC_LANGUAGE_BASIC && c=='"' && next=='"'){i+=2;continue;}
            if(lang!=UMC_LANGUAGE_BASIC && c=='\\'){i+=next?2U:1U;continue;}
            if(c==quote)state=0;
            ++i;continue;
        }
        /* REM is a BASIC statement comment only at the beginning of a line
         * or after a colon. Matching a bounded, case-insensitive keyword avoids
         * swallowing identifiers such as Remember or unrelated expressions. */
        if(lang==UMC_LANGUAGE_BASIC && basic_statement_start && basic_rem_at(source,length,i)) {
            state=1;i+=3U;continue;
        }
        if(c=='<'&&next=='?') {
            if(tagged)return fail_at(source,length,d,i,"Language tags cannot nest");
            if(i>start && !add(regions,capacity,&used,lang,start,i,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,i);
            size_t name=i+2,j=name;while(j<length && source[j]>='a'&&source[j]<='z')++j;
            if(j<length && source[j]>='0'&&source[j]<='9')while(j<length && source[j]>='0'&&source[j]<='9')++j;
            if(j==length || !space(source[j]))return fail_at(source,length,d,i,"A language tag requires trailing whitespace");
            if(same(source+name,j-name,"umicom"))lang=UMC_LANGUAGE_UMICOM;
            else if(same(source+name,j-name,"c23"))lang=UMC_LANGUAGE_C23;
            else if(same(source+name,j-name,"basic"))lang=UMC_LANGUAGE_BASIC;
            else if(same(source+name,j-name,"java"))lang=UMC_LANGUAGE_JAVA;
            else return fail_at(source,length,d,i,"Unknown language tag");
            tag=i;start=j;i=j;tagged=1;basic_statement_start=1;continue;
        }
        if(c=='?'&&next=='>') {
            if(!tagged)return fail_at(source,length,d,i,"Closing tag has no opening tag");
            if(!add(regions,capacity,&used,lang,start,i,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,i);
            tagged=0;lang=UMC_LANGUAGE_UMICOM;i+=2;start=i;tag=i;continue;
        }
        if(lang==UMC_LANGUAGE_BASIC && c=='\''){state=1;++i;continue;}
        if(lang!=UMC_LANGUAGE_BASIC && c=='/'&&next=='/'){state=1;i+=2;continue;}
        if(lang!=UMC_LANGUAGE_BASIC && c=='/'&&next=='*'){state=2;quote_start=i;i+=2;continue;}
        if(c=='"' || (lang!=UMC_LANGUAGE_BASIC && c=='\'')){state=c=='"'?3:4;quote_start=i;}
        if(lang==UMC_LANGUAGE_BASIC) {
            if(c=='\r'||c=='\n'||c==':') basic_statement_start=1;
            else if(!space(c)) basic_statement_start=0;
        }
        ++i;
    }
    if(state==2||state==3||state==4)return fail_at(source,length,d,quote_start,"Unterminated quoted text or comment");
    if(tagged)return fail_at(source,length,d,tag,"Language tag is missing its closing delimiter");
    if(start<length && !add(regions,capacity,&used,lang,start,length,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,start);
    *count=used;return umc_fail(d,UMC_OK,0);
}

#endif
/* The BASIC-capable scanner is retained for review. The active scanner adds
 * an explicit D lexical envelope and bounded comment/raw-string protection;
 * no foreign frontend or runtime is introduced. */
#if 0
UmcStatus umc_source_regions(const char *source,size_t length,UmcSourceRegion *regions,size_t capacity,size_t *count,UmcDiagnostic *d)
{
    if(!count)return umc_fail(d,UMC_INVALID_ARGUMENT,0);
    *count=0;
    if(!source || !regions)return umc_fail(d,UMC_INVALID_ARGUMENT,0);
    if(length>UMC_SOURCE_LIMIT)return umc_fail(d,UMC_SOURCE_TOO_LARGE,0);
    for(size_t n=0;n<length;++n)if(source[n]=='\0')return fail_at(source,length,d,n,"NUL is not a source character");
    size_t used=0,start=0,tag=0,i=0,quote_start=0;
    size_t basic_block_depth=0U;
    int basic_statement_start=1;
    int tagged=0,state=0; /* 1 line comment, 2 C block, 3 string, 4 character, 5 BASIC block. */
    UmcSourceLanguage lang=UMC_LANGUAGE_UMICOM;
    while(i<length) {
        char c=source[i],next=i+1<length?source[i+1]:'\0';
        if(state==1){if(c=='\r'||c=='\n'){state=0;basic_statement_start=1;}++i;continue;}
        if(state==2){if(c=='*'&&next=='/'){state=0;i+=2;}else ++i;continue;}
        /* BASIC block markers nest even within apparent quoted comment text.
         * A counter avoids recursion. Newlines inside a block do not begin a
         * new statement, so the enclosing REM-admission state is unchanged. */
        if(state==5){
            if(c=='/'&&next=='\''){
                if(basic_block_depth==UMC_BASIC_COMMENT_DEPTH_LIMIT)
                    return fail_at(source,length,d,i,"BASIC block comment nesting limit exceeded");
                ++basic_block_depth;i+=2U;
            } else if(c=='\''&&next=='/'){
                --basic_block_depth;i+=2U;
                if(basic_block_depth==0U)state=0;
            } else ++i;
            continue;
        }
        if(state==3||state==4){
            const char quote=state==3?'"':'\'';
            if(lang==UMC_LANGUAGE_BASIC && c=='"' && next=='"'){i+=2;continue;}
            if(lang!=UMC_LANGUAGE_BASIC && c=='\\'){i+=next?2U:1U;continue;}
            if(c==quote)state=0;
            ++i;continue;
        }
        /* REM is a BASIC statement comment only at the beginning of a line
         * or after a colon. Matching a bounded, case-insensitive keyword avoids
         * swallowing identifiers such as Remember or unrelated expressions. */
        if(lang==UMC_LANGUAGE_BASIC && basic_statement_start && basic_rem_at(source,length,i)) {
            state=1;i+=3U;continue;
        }
        if(c=='<'&&next=='?') {
            if(tagged)return fail_at(source,length,d,i,"Language tags cannot nest");
            if(i>start && !add(regions,capacity,&used,lang,start,i,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,i);
            size_t name=i+2,j=name;while(j<length && source[j]>='a'&&source[j]<='z')++j;
            if(j<length && source[j]>='0'&&source[j]<='9')while(j<length && source[j]>='0'&&source[j]<='9')++j;
            if(j==length || !space(source[j]))return fail_at(source,length,d,i,"A language tag requires trailing whitespace");
            if(same(source+name,j-name,"umicom"))lang=UMC_LANGUAGE_UMICOM;
            else if(same(source+name,j-name,"c23"))lang=UMC_LANGUAGE_C23;
            else if(same(source+name,j-name,"basic"))lang=UMC_LANGUAGE_BASIC;
            else if(same(source+name,j-name,"java"))lang=UMC_LANGUAGE_JAVA;
            else return fail_at(source,length,d,i,"Unknown language tag");
            tag=i;start=j;i=j;tagged=1;basic_statement_start=1;continue;
        }
        if(c=='?'&&next=='>') {
            if(!tagged)return fail_at(source,length,d,i,"Closing tag has no opening tag");
            if(!add(regions,capacity,&used,lang,start,i,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,i);
            tagged=0;lang=UMC_LANGUAGE_UMICOM;i+=2;start=i;tag=i;continue;
        }
        if(lang==UMC_LANGUAGE_BASIC && c=='/'&&next=='\''){
            state=5;basic_block_depth=1U;quote_start=i;i+=2U;continue;
        }
        if(lang==UMC_LANGUAGE_BASIC && c=='\''){state=1;++i;continue;}
        if(lang!=UMC_LANGUAGE_BASIC && c=='/'&&next=='/'){state=1;i+=2;continue;}
        if(lang!=UMC_LANGUAGE_BASIC && c=='/'&&next=='*'){state=2;quote_start=i;i+=2;continue;}
        if(c=='"' || (lang!=UMC_LANGUAGE_BASIC && c=='\'')){state=c=='"'?3:4;quote_start=i;}
        if(lang==UMC_LANGUAGE_BASIC) {
            if(c=='\r'||c=='\n'||c==':') basic_statement_start=1;
            else if(!space(c)) basic_statement_start=0;
        }
        ++i;
    }
    if(state==2||state==3||state==4||state==5)return fail_at(source,length,d,quote_start,"Unterminated quoted text or comment");
    if(tagged)return fail_at(source,length,d,tag,"Language tag is missing its closing delimiter");
    if(start<length && !add(regions,capacity,&used,lang,start,length,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,start);
    *count=used;return umc_fail(d,UMC_OK,0);
}

#endif

/* Prefix recognition must not reinterpret the final letter of an identifier.
 * Treat high bytes as identifier continuations for this conservative boundary;
 * no Unicode decoding or D identifier validation is claimed. */
static int region_identifier_byte(char c)
{
    const unsigned char byte=(unsigned char)c;
    return (byte>='a' && byte<='z') || (byte>='A' && byte<='Z') ||
        (byte>='0' && byte<='9') || byte=='_' || byte>=128U;
}

UmcStatus umc_source_regions(const char *source,size_t length,UmcSourceRegion *regions,size_t capacity,size_t *count,UmcDiagnostic *d)
{
    if(!count)return umc_fail(d,UMC_INVALID_ARGUMENT,0);
    *count=0;
    if(!source || !regions)return umc_fail(d,UMC_INVALID_ARGUMENT,0);
    if(length>UMC_SOURCE_LIMIT)return umc_fail(d,UMC_SOURCE_TOO_LARGE,0);
    for(size_t n=0;n<length;++n)if(source[n]=='\0')return fail_at(source,length,d,n,"NUL is not a source character");
    size_t used=0,start=0,tag=0,i=0,quote_start=0;
    size_t basic_block_depth=0U, d_block_depth=0U;
    int basic_statement_start=1;
    int tagged=0,state=0; /* 1 line comment, 2 C block, 3 string, 4 character, 5 BASIC block; 6 D nested block; 7/8 D raw quotes. */
    UmcSourceLanguage lang=UMC_LANGUAGE_UMICOM;
    while(i<length) {
        char c=source[i],next=i+1<length?source[i+1]:'\0';
        if(state==1){if(c=='\r'||c=='\n'){state=0;basic_statement_start=1;}++i;continue;}
        if(state==2){if(c=='*'&&next=='/'){state=0;i+=2;}else ++i;continue;}
        /* BASIC block markers nest even within apparent quoted comment text.
         * A counter avoids recursion. Newlines inside a block do not begin a
         * new statement, so the enclosing REM-admission state is unchanged. */
        if(state==5){
            if(c=='/'&&next=='\''){
                if(basic_block_depth==UMC_BASIC_COMMENT_DEPTH_LIMIT)
                    return fail_at(source,length,d,i,"BASIC block comment nesting limit exceeded");
                ++basic_block_depth;i+=2U;
            } else if(c=='\''&&next=='/'){
                --basic_block_depth;i+=2U;
                if(basic_block_depth==0U)state=0;
            } else ++i;
            continue;
        }

        /* D nested comments treat quotes as ordinary comment bytes. Raw
         * strings instead ignore escapes and close only on their delimiter.
         * All nesting is bounded and iterative, without recursive allocation. */
        if(state==6) {
            if(c=='/' && next=='+') {
                if(d_block_depth==UMC_D_COMMENT_DEPTH_LIMIT)
                    return fail_at(source,length,d,i,"D block comment nesting limit exceeded");
                ++d_block_depth;i+=2U;
            } else if(c=='+' && next=='/') {
                --d_block_depth;i+=2U;if(d_block_depth==0U)state=0;
            } else ++i;
            continue;
        }
        if(state==7 || state==8) {
            if(c==(state==7?'"':(char)0x60))state=0;
            ++i;continue;
        }
        if(state==3||state==4){
            const char quote=state==3?'"':'\'';
            if(lang==UMC_LANGUAGE_BASIC && c=='"' && next=='"'){i+=2;continue;}
            if(lang!=UMC_LANGUAGE_BASIC && c=='\\'){i+=next?2U:1U;continue;}
            if(c==quote)state=0;
            ++i;continue;
        }
        /* REM is a BASIC statement comment only at the beginning of a line
         * or after a colon. Matching a bounded, case-insensitive keyword avoids
         * swallowing identifiers such as Remember or unrelated expressions. */
        if(lang==UMC_LANGUAGE_BASIC && basic_statement_start && basic_rem_at(source,length,i)) {
            state=1;i+=3U;continue;
        }

        if(lang==UMC_LANGUAGE_D) {
            if(c=='/' && next=='+') {
                state=6;d_block_depth=1U;quote_start=i;i+=2U;continue;
            }
            if(c==(char)0x60) {state=8;quote_start=i;++i;continue;}
            if((i==0U || !region_identifier_byte(source[i-1U]))) {
                if(c=='r' && next=='"') {state=7;quote_start=i;i+=2U;continue;}
                /* Token/delimited strings and interpolation need a D token
                 * parser. Refuse them explicitly before treating their quotes
                 * as ordinary strings; do not report a false region boundary. */
                if((c=='q' && (next=='"' || next=='{')) ||
                   (c=='x' && next=='"') ||
                   (c=='i' && (next=='"' || next==(char)0x60)))
                    return fail_at(source,length,d,i,"Unsupported D string form; use a separate D source file");
            }
        }
        if(c=='<'&&next=='?') {
            if(tagged)return fail_at(source,length,d,i,"Language tags cannot nest");
            if(i>start && !add(regions,capacity,&used,lang,start,i,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,i);
            size_t name=i+2,j=name;while(j<length && source[j]>='a'&&source[j]<='z')++j;
            if(j<length && source[j]>='0'&&source[j]<='9')while(j<length && source[j]>='0'&&source[j]<='9')++j;
            if(j==length || !space(source[j]))return fail_at(source,length,d,i,"A language tag requires trailing whitespace");
            if(same(source+name,j-name,"umicom"))lang=UMC_LANGUAGE_UMICOM;
            else if(same(source+name,j-name,"c23"))lang=UMC_LANGUAGE_C23;
            else if(same(source+name,j-name,"basic"))lang=UMC_LANGUAGE_BASIC;
            else if(same(source+name,j-name,"java"))lang=UMC_LANGUAGE_JAVA;
            else if(same(source+name,j-name,"d"))lang=UMC_LANGUAGE_D;
            else return fail_at(source,length,d,i,"Unknown language tag");
            tag=i;start=j;i=j;tagged=1;basic_statement_start=1;continue;
        }
        if(c=='?'&&next=='>') {
            if(!tagged)return fail_at(source,length,d,i,"Closing tag has no opening tag");
            if(!add(regions,capacity,&used,lang,start,i,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,i);
            tagged=0;lang=UMC_LANGUAGE_UMICOM;i+=2;start=i;tag=i;continue;
        }
        if(lang==UMC_LANGUAGE_BASIC && c=='/'&&next=='\''){
            state=5;basic_block_depth=1U;quote_start=i;i+=2U;continue;
        }
        if(lang==UMC_LANGUAGE_BASIC && c=='\''){state=1;++i;continue;}
        if(lang!=UMC_LANGUAGE_BASIC && c=='/'&&next=='/'){state=1;i+=2;continue;}
        if(lang!=UMC_LANGUAGE_BASIC && c=='/'&&next=='*'){state=2;quote_start=i;i+=2;continue;}
        if(c=='"' || (lang!=UMC_LANGUAGE_BASIC && c=='\'')){state=c=='"'?3:4;quote_start=i;}
        if(lang==UMC_LANGUAGE_BASIC) {
            if(c=='\r'||c=='\n'||c==':') basic_statement_start=1;
            else if(!space(c)) basic_statement_start=0;
        }
        ++i;
    }
    if(state==2||state==3||state==4||state==5||state==6||state==7||state==8)return fail_at(source,length,d,quote_start,"Unterminated quoted text or comment");
    if(tagged)return fail_at(source,length,d,tag,"Language tag is missing its closing delimiter");
    if(start<length && !add(regions,capacity,&used,lang,start,length,tag))return umc_fail(d,UMC_BUFFER_TOO_SMALL,start);
    *count=used;return umc_fail(d,UMC_OK,0);
}
