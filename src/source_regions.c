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
const char *umc_source_language_name(UmcSourceLanguage language)
{
    switch(language){case UMC_LANGUAGE_UMICOM:return "umicom";case UMC_LANGUAGE_C23:return "c23";
        case UMC_LANGUAGE_BASIC:return "basic";case UMC_LANGUAGE_JAVA:return "java";default:return "unknown";}
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
