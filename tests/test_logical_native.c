/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_logical_native.c
 *
 * PURPOSE:
 *   Compare the same defined integer program bodies compiled by native C and by Umicom C.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
/* Stringizing a body keeps the native and interpreted inputs together. Each
 * fixture stays within their common subset and avoids evaluated C undefined
 * behavior. A successful comparison is evidence for this corpus only. */
#define PROGRAMS(X) \
 X(guard_zero, { int divisor=0; return divisor!=0 && 42/divisor>1; }) \
 X(guard_nonzero, { int divisor=7; return divisor!=0 && 42/divisor>1; }) \
 X(fallback, { int cached=9; int divisor=0; return cached || 42/divisor; }) \
 X(precedence, { int a=1; int b=0; int c=0; return a || (b && c); }) \
 X(grouping, { int a=1; int b=0; int c=0; return (a || b) && c; }) \
 X(chain, { int a=0; int b=0; int c=-7; return a || b || c; }) \
 X(assignment, { int a=2; int b=9; int x=a && b; x=x+(!a || b); return x; }) \
 X(negative, { int a=-7; int b=-3; return a<b && b<0; }) \
 X(nested, { int a=0; int b=1; int d=0; return (a || b) && (b || 1/d); }) \
 X(select_value, { int a=-1; return a ? -9 : 42; }) \
 X(select_guard, { int d=0; return d ? 42/d : 7; }) \
 X(select_right, { int a=1; int b=0; int c=1; return a ? b : c ? 7 : 9; }) \
 X(select_middle, { int a=1; int b=0; return a ? b ? 7 : 9 : 11; }) \
 X(select_assign, { int d=7; int x=d?42/d:0; x=x>5?x*7:0; return x; })
#define DEFINE(name,body) static int native_##name(void) body
PROGRAMS(DEFINE)
#undef DEFINE
struct Fixture { const char *name; const char *source; int (*native)(void); };
#define ROW(name,body) {#name,"int main(void)" #body,native_##name},
static const struct Fixture fixtures[]={PROGRAMS(ROW)};
#undef ROW
int main(void)
{
    for(size_t i=0U;i<sizeof fixtures/sizeof fixtures[0];++i) {
        UmcInstruction code[UMC_PROGRAM_LIMIT]; UmcDiagnostic d; UmcResult r; size_t count=0U;
        if(umc_compile(fixtures[i].source,strlen(fixtures[i].source),code,UMC_PROGRAM_LIMIT,&count,&d)!=UMC_OK) {
            fprintf(stderr,"compile fixture %s at %zu\n",fixtures[i].name,d.source_offset);return 1;
        }
        CHECK(umc_execute(code,count,count,&r,&d)==UMC_OK);
        CHECK(r.value==fixtures[i].native());
    }
    return 0;
}
