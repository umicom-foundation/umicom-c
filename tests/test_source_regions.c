/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_source_regions.c
 *
 * PURPOSE:
 *   Check language envelope spans, protected delimiters, malformed tags and bounded outputs.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
#include "umicom_c/source_regions.h"
static int regions(void)
{
    const char *s="int a;<?basic\nDIM n AS INTEGER = 2\n?><?java\nint j = 3;\n?><?c23\nint k;\n?>";
    UmcSourceRegion r[8];size_t n=99;UmcDiagnostic d;
    CHECK(umc_source_regions(s,strlen(s),r,8,&n,&d)==UMC_OK && n==4);
    CHECK(r[0].language==UMC_LANGUAGE_UMICOM && r[0].offset==0 && r[0].length==6);
    CHECK(r[1].language==UMC_LANGUAGE_BASIC && r[2].language==UMC_LANGUAGE_JAVA && r[3].language==UMC_LANGUAGE_C23);
    for(size_t i=1;i<n;++i){CHECK(s[r[i].tag_offset]=='<' && s[r[i].offset]=='\n');CHECK(s[r[i].offset+r[i].length]=='?');}
    CHECK(!strcmp(umc_source_language_name(UMC_LANGUAGE_BASIC),"basic"));
    return 0;
}
static int protected(void)
{
    const char *cases[]={
        "<?java\nString s=\"?>\"; // ?>\n /* ?> */ int n=2; ?>",
        "<?basic\nvalue = \"a\"\"?>b\" ' ?>\n?>",
        "/* <?java */ int main(void){return 42;}// <?basic"
    };
    for(size_t i=0;i<3;++i){UmcSourceRegion r[4];size_t n;UmcDiagnostic d;
        CHECK(umc_source_regions(cases[i],strlen(cases[i]),r,4,&n,&d)==UMC_OK && n==1);}
    return 0;
}
static int malformed(void)
{
    const char *cases[]={"<?php code ?>","<?java int x;","<?java <?basic x ?> ?>","?>","<?java?>","\"oops","/*oops"};
    for(size_t i=0;i<sizeof cases/sizeof cases[0];++i){UmcSourceRegion r[8];size_t n=99;UmcDiagnostic d;
        CHECK(umc_source_regions(cases[i],strlen(cases[i]),r,8,&n,&d)==UMC_SYNTAX_ERROR && n==0);
        CHECK(d.line>=1 && d.column>=1);}
    return 0;
}
static int bounds(void)
{
    UmcSourceRegion r[UMC_REGION_LIMIT];size_t n=99;UmcDiagnostic d;
    const char s[]={'x',0,'y'};
    CHECK(umc_source_regions(s,sizeof s,r,UMC_REGION_LIMIT,&n,&d)==UMC_SYNTAX_ERROR && d.source_offset==1 && n==0);
    CHECK(umc_source_regions("x",1,r,0,&n,&d)==UMC_BUFFER_TOO_SMALL && n==0);
    CHECK(umc_source_regions("",0,r,1,&n,&d)==UMC_OK && n==0);
    CHECK(umc_source_regions(NULL,0,r,1,&n,&d)==UMC_INVALID_ARGUMENT && n==0);
    CHECK(umc_source_regions("",UMC_SOURCE_LIMIT+1,r,1,&n,&d)==UMC_SOURCE_TOO_LARGE);
    char source[1024];size_t used=0;const char *segment="<?umicom ?>";
    for(size_t i=0;i<UMC_REGION_LIMIT+1;++i){memcpy(source+used,segment,strlen(segment));used+=strlen(segment);}
    CHECK(umc_source_regions(source,used,r,UMC_REGION_LIMIT,&n,&d)==UMC_BUFFER_TOO_SMALL && n==0);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"regions"))return regions();
    if(!strcmp(argv[1],"protected"))return protected();
    if(!strcmp(argv[1],"malformed"))return malformed();
    if(!strcmp(argv[1],"bounds"))return bounds();
    return 2;
}
