/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_basic_block_regions.c
 *
 * PURPOSE:
 *   Verify nested BASIC comment protection, limits, diagnostic offsets and language isolation.
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
    UmcSourceRegion r[4]; size_t count=99U; UmcDiagnostic d;
    CHECK(umc_source_regions(source,strlen(source),r,4U,&count,&d)==UMC_OK);
    CHECK(count==1U && r[0].language==UMC_LANGUAGE_BASIC);
    CHECK(r[0].offset==7U && r[0].tag_offset==0U);
    CHECK(r[0].offset+r[0].length==strlen(source)-2U);
    return 0;
}
static int refused(const char *source,size_t length,size_t offset)
{
    UmcSourceRegion r[4]; size_t count=99U; UmcDiagnostic d;
    CHECK(umc_source_regions(source,length,r,4U,&count,&d)==UMC_SYNTAX_ERROR);
    CHECK(count==0U && d.source_offset==offset && d.line>=1U && d.column>=1U);
    return 0;
}
static size_t nesting(char *source,size_t depth)
{
    size_t used=7U;
    memcpy(source,"<?basic",used); source[used++]='\n';
    for(size_t i=0U;i<depth;++i){memcpy(source+used,"/'",2U);used+=2U;}
    memcpy(source+used," ?> <?java ",11U);used+=11U;
    for(size_t i=0U;i<depth;++i){memcpy(source+used,"'/",2U);used+=2U;}
    memcpy(source+used," ?>",3U);used+=3U; source[used]='\0';
    return used;
}
int main(int argc,char **argv)
{
    CHECK(argc==2);
    if(!strcmp(argv[1],"ordinary")){
        CHECK(one("<?basic\n/''/ PRINT 1 ?>")==0);
        CHECK(one("<?basic\n/' ?> <?java \" */\r\n ignored '/ PRINT 1 ?>")==0);
        CHECK(one("<?basic\nDIM x AS /' across\n lines '/ INTEGER ?>")==0);
    }else if(!strcmp(argv[1],"nested")){
        CHECK(one("<?basic\n/' outer /' inner ?> '/ still <?java '/ ?>")==0);
        /* Quotes are comment bytes, not string boundaries while in a block. */
        CHECK(one("<?basic\n/' \"/' nested ?> '/\" outer '/ ?>")==0);
        CHECK(one("<?basic\n/' a '/ /' b '/ ?>")==0);
    }else if(!strcmp(argv[1],"protected-markers")){
        CHECK(one("<?basic\nPRINT \"/' ?>\"\n?>")==0);
        CHECK(one("<?basic\n' /' ?> ignored\n?>")==0);
        CHECK(one("<?basic\nREM /' ?> ignored\n?>")==0);
        CHECK(one("<?basic\n'/ ?> ordinary apostrophe comment\n?>")==0);
    }else if(!strcmp(argv[1],"statement-state")){
        CHECK(one("<?basic\n/' x\n y '/ REM ?> ignored\n?>")==0);
        CHECK(one("<?basic\nPRINT 1 : /' x\n y '/ rEm ?> ignored\n?>")==0);
        CHECK(one("<?basic\nPRINT /' x\n y '/ REM ?>")==0);
        CHECK(one("<?basic\nPRINT /' :\n '/ REM ?>")==0);
        CHECK(one("<?basic\nPRINT /' x '/\nREM ?> ignored\n?>")==0);
    }else if(!strcmp(argv[1],"depth-boundary")){
        char source[512];
        for(size_t depth=1U;depth<=UMC_BASIC_COMMENT_DEPTH_LIMIT;++depth){
            (void)nesting(source,depth);CHECK(one(source)==0);
        }
    }else if(!strcmp(argv[1],"depth-overflow")){
        char source[512]; const size_t n=nesting(source,UMC_BASIC_COMMENT_DEPTH_LIMIT+1U);
        CHECK(refused(source,n,8U+2U*UMC_BASIC_COMMENT_DEPTH_LIMIT)==0);
    }else if(!strcmp(argv[1],"unterminated")){
        const char *cases[]={"<?basic\n/'", "<?basic\n/' outer /' inner '/ ?>", "<?basic\n/' dangling '"};
        for(size_t i=0U;i<sizeof(cases)/sizeof(cases[0]);++i)CHECK(refused(cases[i],strlen(cases[i]),8U)==0);
        const char *partial="head<?basic\n/' ?>";
        CHECK(refused(partial,strlen(partial),12U)==0);
    }else if(!strcmp(argv[1],"bounded-span")){
        /* No terminating NUL: the final slash must not inspect a later byte. */
        const char exact[]={'<','?','b','a','s','i','c','\n','/','\'','x','\'','/',' ','?','>'};
        UmcSourceRegion r[4]; size_t count; UmcDiagnostic d;
        CHECK(umc_source_regions(exact,sizeof(exact),r,4U,&count,&d)==UMC_OK && count==1U);
        CHECK(umc_source_regions(exact,12U,r,4U,&count,&d)==UMC_SYNTAX_ERROR && count==0U && d.source_offset==8U);
        CHECK(umc_source_regions(exact,13U,r,4U,&count,&d)==UMC_SYNTAX_ERROR && count==0U && d.source_offset==0U);
        for(size_t n=0U;n<=sizeof(exact);++n){
            const UmcStatus status=umc_source_regions(exact,n,r,4U,&count,&d);
            CHECK(status==UMC_OK || (status==UMC_SYNTAX_ERROR && count==0U && d.source_offset<=n));
        }
    }else if(!strcmp(argv[1],"language-isolation")){
        CHECK(refused("/'",2U,1U)==0);
        const char *s="<?c23\n/* /' */ int n; ?><?basic\n/' ?> '/ PRINT 1 ?><?java\nString x=\"/'\"; ?>";
        UmcSourceRegion r[4]; size_t count; UmcDiagnostic d;
        CHECK(umc_source_regions(s,strlen(s),r,4U,&count,&d)==UMC_OK && count==3U);
        CHECK(r[0].language==UMC_LANGUAGE_C23 && r[1].language==UMC_LANGUAGE_BASIC && r[2].language==UMC_LANGUAGE_JAVA);
    }else if(!strcmp(argv[1],"source-and-output-bounds")){
        static char source[UMC_SOURCE_LIMIT];
        UmcSourceRegion r[2]; size_t count=99U; UmcDiagnostic d;
        memset(source,'x',sizeof(source));memcpy(source,"<?basic\n/'",10U);
        memcpy(source+sizeof(source)-5U,"'/ ?>",5U);
        CHECK(umc_source_regions(source,sizeof(source),r,2U,&count,&d)==UMC_OK && count==1U);
        CHECK(r[0].offset+r[0].length==sizeof(source)-2U);
        CHECK(umc_source_regions(source,sizeof(source),r,0U,&count,&d)==UMC_BUFFER_TOO_SMALL && count==0U);
        source[20]='\0';
        CHECK(refused(source,sizeof(source),20U)==0);
    }else return 2;
    return 0;
}
