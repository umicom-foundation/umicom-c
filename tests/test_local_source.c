/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_local_source.c
 *
 * PURPOSE:
 *   Exercise local declarations, assignment, lexical scope and independent capacity limits.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
#include "umicom_c/comparison.h"

static int values(void)
{
    static const struct { const char *source; int32_t expected; } cases[] = {
        {"int main(void){int x=6; x=x*7; return x;}",42},
        {"int main(void){int x=0x2a; int y=x==42; return !(!y);}",1},
        {"int main(void){int a=2; int aa=3; int A=5; int _a1=7; return a+aa*A+_a1;}",24},
        {"int main(void){int value=-2147483647-1; return value;}",INT32_MIN},
        {"int main(void){int x=2; int y=x+1; x=9; return y*10+x;}",39},
        {"int main(void){int intx=1; int returnx=2; int voidx=3; int mainx=4; return intx+returnx+voidx+mainx;}",10},
        {"int main(void){int x /* name */ = 2; // assigned below\n x = x + 1; return x;}",3}
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        UmcInstruction p[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; UmcResult r; size_t n=0U;
        CHECK(umc_compile(cases[i].source,strlen(cases[i].source),p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK);
        CHECK(umc_execute(p,n,n,&r,&d)==UMC_OK && r.value==cases[i].expected);
    }
    return 0;
}

static int refusals(void)
{
    static const char *cases[]={
        "int main(void){return missing;}", "int main(void){x=1; return 0;}",
        "int main(void){int x=x; return x;}", "int main(void){int x=y; int y=1; return x;}",
        "int main(void){int x=1; int x=2; return x;}", "int main(void){int x; return x;}",
        "int main(void){int x=1,y=2; return x;}", "int main(void){int x=1; return x=2;}",
        "int main(void){int x=1; int y=2; x=y=3; return x;}",
        "int main(void){int x=1; x+=1; return x;}", "int main(void){int x=1; x++; return x;}",
        "int main(void){{int x=1;} return 0;}", "int main(void){int x[2]=1; return 0;}",
        "int main(void){int *x=0; return 0;}", "int main(void){int x=1; return x; x=2;}",
        "int main(void){int x=1; return X;}", "int main(void){int return=1; return 0;}",
        "int main(void){int x=1 x=2; return x;}", "int main(void){int x=; return x;}",
        "int main(void){int x=1; return x();}", "int main(void){int x=1; int y=!x=0; return y;}"
    };
    for(size_t i=0U;i<sizeof cases/sizeof cases[0];++i) {
        UmcInstruction p[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; size_t n=91U;
        CHECK(umc_compile(cases[i],strlen(cases[i]),p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR);
        CHECK(n==0U && d.line!=0U && d.column!=0U);
    }
    return 0;
}

static int bounds(void)
{
    char source[4096],name[65]; UmcInstruction p[UMC_PROGRAM_LIMIT]; UmcDiagnostic d;
    size_t n=0U; memset(name,'a',63U); name[63]='\0';
    int written=snprintf(source,sizeof source,"int main(void){int %s=42; return %s;}",name,name);
    CHECK(written>0 && (size_t)written<sizeof source);
    CHECK(umc_compile(source,(size_t)written,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK);
    name[63]='b';name[64]='\0';
    written=snprintf(source,sizeof source,"int main(void){int %s=42; return 0;}",name);
    CHECK(written>0 && (size_t)written<sizeof source);
    CHECK(umc_compile(source,(size_t)written,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR && n==0U);
    size_t used=(size_t)snprintf(source,sizeof source,"int main(void){");
    for(size_t i=0U;i<UMC_LOCAL_LIMIT;++i) {
        written=snprintf(source+used,sizeof source-used,"int v%zu=%zu;",i,i);
        CHECK(written>0 && (size_t)written<sizeof source-used); used+=(size_t)written;
    }
    const size_t declarations=used;
    written=snprintf(source+used,sizeof source-used,"return v31;}");
    CHECK(written>0 && (size_t)written<sizeof source-used); used+=(size_t)written;
    CHECK(umc_compile(source,used,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_OK && n==66U);
    UmcResult r; CHECK(umc_execute(p,n,n,&r,&d)==UMC_OK && r.value==31);
    written=snprintf(source+declarations,sizeof source-declarations,"int extra=0; return v31;}");
    CHECK(written>0 && (size_t)written<sizeof source-declarations);
    CHECK(umc_compile(source,declarations+(size_t)written,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_LOCAL_CAPACITY && n==0U);
    const char limited[]="int main(void){int x=1; return x;}";
    CHECK(umc_compile(limited,sizeof limited-1U,p,3U,&n,&d)==UMC_PROGRAM_TOO_LARGE && n==0U);
    /* Assigning repeatedly consumes instructions, not extra symbol slots. */
    used=(size_t)snprintf(source,sizeof source,"int main(void){int x=0;");
    for(size_t i=0U;i<127U;++i) {
        written=snprintf(source+used,sizeof source-used,"x=1;");
        CHECK(written>0 && (size_t)written<sizeof source-used);used+=(size_t)written;
    }
    written=snprintf(source+used,sizeof source-used,"return x;}");
    CHECK(written>0 && (size_t)written<sizeof source-used);used+=(size_t)written;
    CHECK(umc_compile(source,used,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_PROGRAM_TOO_LARGE && n==0U);
    return 0;
}

static int spans(void)
{
    const char source[]="int main(void){\r\n  int x=1;\r\n  x=y; return x;}";
    UmcInstruction p[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; size_t n=0U;
    CHECK(umc_compile(source,sizeof source-1U,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR);
    CHECK(d.source_offset==(size_t)(strstr(source,"y;")-source) && d.line==3U && d.column==5U);
    CHECK(strstr(d.message,"declared")!=NULL);
    const char span[]="int main(void){int a=42;return a;}ignored";
    const size_t length=(size_t)(strstr(span,"ignored")-span);
    CHECK(umc_compile(span,length,p,UMC_PROGRAM_LIMIT,&n,NULL)==UMC_OK);
    UmcResult r; CHECK(umc_execute(p,n,n,&r,NULL)==UMC_OK && r.value==42);
    CHECK(umc_compile(span,sizeof span-1U,p,UMC_PROGRAM_LIMIT,&n,&d)==UMC_SYNTAX_ERROR && n==0U);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(strcmp(argv[1],"values")==0)return values();
    if(strcmp(argv[1],"refusals")==0)return refusals();
    if(strcmp(argv[1],"bounds")==0)return bounds();
    if(strcmp(argv[1],"spans")==0)return spans();
    return 2;
}
