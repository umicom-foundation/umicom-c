/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_d_regions.c
 *
 * PURPOSE:
 *   Check D envelope boundaries, raw strings, nested comments, refusal offsets and stable language identities.
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
static int one(const char *source)
{
    UmcSourceRegion regions[4];size_t count=99U;UmcDiagnostic diagnostic;
    CHECK(umc_source_regions(source,strlen(source),regions,4U,&count,&diagnostic)==UMC_OK);
    CHECK(count==1U && regions[0].language==UMC_LANGUAGE_D && regions[0].offset==3U);
    CHECK(regions[0].tag_offset==0U && regions[0].offset+regions[0].length==strlen(source)-2U);
    return 0;
}
static int refused(const char *source,size_t size,size_t offset,const char *message)
{
    UmcSourceRegion regions[4];size_t count=99U;UmcDiagnostic diagnostic;
    CHECK(umc_source_regions(source,size,regions,4U,&count,&diagnostic)==UMC_SYNTAX_ERROR);
    CHECK(count==0U && diagnostic.source_offset==offset);
    CHECK(diagnostic.line>=1U && diagnostic.column>=1U);
    CHECK(strstr(diagnostic.message,message)!=NULL);
    return 0;
}
static size_t nested(char *source,size_t depth)
{
    size_t used=4U;
    memcpy(source,"<?d ",used);
    for(size_t i=0;i<depth;++i){memcpy(source+used,"/+",2U);used+=2U;}
    memcpy(source+used," ?> <?java ",11U);used+=11U;
    for(size_t i=0;i<depth;++i){memcpy(source+used,"+/",2U);used+=2U;}
    memcpy(source+used," ?>",3U);used+=3U;source[used]=0;
    return used;
}
int main(int argc,char **argv)
{
    CHECK(argc==2);
    const char *mode=argv[1];
    if(!strcmp(mode,"identity")) {
        CHECK(UMC_LANGUAGE_UMICOM==0 && UMC_LANGUAGE_C23==1 && UMC_LANGUAGE_BASIC==2 &&
            UMC_LANGUAGE_JAVA==3 && UMC_LANGUAGE_D==4);
        CHECK(!strcmp(umc_source_language_name(UMC_LANGUAGE_D),"d"));
        CHECK(!strcmp(umc_source_language_name((UmcSourceLanguage)99),"unknown"));
        CHECK(one("<?d ?>")==0);
    } else if(!strcmp(mode,"ordinary")) {
        const char *cases[]={"<?d\nint n = 7; ?>",
            "<?d\n/+ comment ?> <?java +/ int n; ?>",
            "<?d\n/+ /+ inside ?> +/ outside <?basic +/ ?>",
            "<?d\n/+ \" /+ ?> +/ \" +/ ?>"};
        for(size_t i=0;i<sizeof cases/sizeof cases[0];++i)CHECK(one(cases[i])==0);
    } else if(!strcmp(mode,"strings")) {
        const char *cases[]={"<?d\nstring a = \"escaped \\\" ?>\"; ?>",
            "<?d\nauto c = '?'; auto s = \"?> /+ +/ <?java\"; ?>",
            "<?d\nauto s = r\"backslash \\ ?> <?java /+\"; ?>",
            "<?d\nauto s = `\" ?> <?basic /+ \\`; ?>"};
        for(size_t i=0;i<sizeof cases/sizeof cases[0];++i)CHECK(one(cases[i])==0);
    } else if(!strcmp(mode,"comments")) {
        const char *cases[]={"<?d\n// /+ ?>\n?>",
            "<?d\n/* /+ ?> */ ?>",
            "<?d\n/+ /* // \" ?> +/ ?>",
            "<?d\n/+ /+ */ ?> +/ +/ ?>"};
        for(size_t i=0;i<sizeof cases/sizeof cases[0];++i)CHECK(one(cases[i])==0);
    } else if(!strcmp(mode,"prefix-boundary")) {
        const char *cases[]={"<?d\nwordq\"?>\"; ?>",
            "<?d\nwordx\"?>\"; ?>",
            "<?d\nwordi\"?>\"; ?>"};
        for(size_t i=0;i<sizeof cases/sizeof cases[0];++i)CHECK(one(cases[i])==0);
    } else if(!strcmp(mode,"unsupported-strings")) {
        const char *cases[]={"<?d\nq\"delimiter ?> delimiter\" ?>",
            "<?d\nq{ ?> } ?>",
            "<?d\nx\"1234\" ?>",
            "<?d\ni\"hello $(1)\" ?>",
            "<?d\ni`hello` ?>"};
        for(size_t i=0;i<sizeof cases/sizeof cases[0];++i)
            CHECK(refused(cases[i],strlen(cases[i]),4U,"Unsupported D string form")==0);
    } else if(!strcmp(mode,"depth-boundary")) {
        char source[512];
        for(size_t i=1;i<=UMC_D_COMMENT_DEPTH_LIMIT;++i){(void)nested(source,i);CHECK(one(source)==0);}
    } else if(!strcmp(mode,"depth-overflow")) {
        char source[512];const size_t size=nested(source,UMC_D_COMMENT_DEPTH_LIMIT+1U);
        CHECK(refused(source,size,4U+2U*UMC_D_COMMENT_DEPTH_LIMIT,"nesting limit")==0);
    } else if(!strcmp(mode,"unterminated")) {
        const char *cases[]={"<?d\n/+",
            "<?d\n/+ /+ nested +/ ?>",
            "<?d\nr\"?>",
            "<?d\n`?>",
            "<?d\n\"?>",
            "<?d\n'x"};
        for(size_t i=0;i<sizeof cases/sizeof cases[0];++i)
            CHECK(refused(cases[i],strlen(cases[i]),4U,"Unterminated")==0);
    } else if(!strcmp(mode,"bounded-span")) {
        const char exact[]={60,63,100,32,47,43,120,43,47,32,63,62};
        UmcSourceRegion regions[4];size_t count;UmcDiagnostic diagnostic;
        CHECK(umc_source_regions(exact,sizeof exact,regions,4U,&count,&diagnostic)==UMC_OK && count==1U);
        CHECK(refused(exact,8U,4U,"Unterminated")==0);
        for(size_t n=0;n<=sizeof exact;++n) {
            const UmcStatus status=umc_source_regions(exact,n,regions,4U,&count,&diagnostic);
            CHECK(status==UMC_OK || (status==UMC_SYNTAX_ERROR && count==0U && diagnostic.source_offset<=n));
        }
    } else if(!strcmp(mode,"language-isolation")) {
        const char *source="<?d /+ ?> +/ ?><?basic /' ?> '/ ?><?java /* ?> */ ?><?c23 /+ ?>";
        UmcSourceRegion regions[4];size_t count;UmcDiagnostic diagnostic;
        CHECK(umc_source_regions(source,strlen(source),regions,4U,&count,&diagnostic)==UMC_OK && count==4U);
        CHECK(regions[0].language==UMC_LANGUAGE_D && regions[1].language==UMC_LANGUAGE_BASIC &&
            regions[2].language==UMC_LANGUAGE_JAVA && regions[3].language==UMC_LANGUAGE_C23);
        CHECK(refused("<?d /+ ?>",9U,4U,"Unterminated")==0);
    } else if(!strcmp(mode,"source-bounds")) {
        static char source[UMC_SOURCE_LIMIT];
        UmcSourceRegion regions[2];size_t count;UmcDiagnostic diagnostic;
        memset(source,'x',sizeof source);memcpy(source,"<?d /+",6U);
        memcpy(source+sizeof source-5U,"+/ ?>",5U);
        CHECK(umc_source_regions(source,sizeof source,regions,2U,&count,&diagnostic)==UMC_OK && count==1U);
        CHECK(regions[0].offset+regions[0].length==sizeof source-2U);
        CHECK(umc_source_regions(source,sizeof source,regions,0U,&count,&diagnostic)==UMC_BUFFER_TOO_SMALL && count==0U);
        source[20]=0;CHECK(refused(source,sizeof source,20U,"NUL")==0);
    } else if(!strcmp(mode,"tags")) {
        CHECK(refused("<?D ?>",6U,0U,"trailing whitespace")==0);
        CHECK(refused("<?d?>",5U,0U,"trailing whitespace")==0);
        CHECK(refused("<?d <?d ?> ?>",13U,4U,"cannot nest")==0);
        CHECK(refused("<?d int n;",10U,0U,"missing its closing")==0);
    } else if(!strcmp(mode,"classification-only")) {
        CHECK(one("<?d this is not a valid D program ?>")==0);
/* Use the public caller-owned instruction array contract to prove an envelope is not accepted by the integer compiler; retain the failed draft call for review. The former implementation is retained for owner review. */
#if 0
        UmcProgram program;UmcDiagnostic diagnostic;
        const char *source="<?d int main(){return 7;} ?>";
        CHECK(umc_compile(source,strlen(source),&program,&diagnostic)!=UMC_OK);
#endif
        UmcInstruction program[UMC_PROGRAM_LIMIT];UmcDiagnostic diagnostic;size_t count=99U;
        const char *source="<?d int main(){return 7;} ?>";
        CHECK(umc_compile(source,strlen(source),program,UMC_PROGRAM_LIMIT,&count,&diagnostic)!=UMC_OK);
        CHECK(count==0U);
    } else return 2;
    return 0;
}
