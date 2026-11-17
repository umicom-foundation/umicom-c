/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_managed_heap.c
 *
 * PURPOSE:
 *   Check explicit-root reachability, cycle reclamation, stale handles and failed-operation atomicity.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "test.h"
#include "umicom_c/managed_heap.h"
static int cycle(void)
{
    UmcManagedHeap h;umc_heap_init(&h);UmcHandle a=0,b=0,c=0;
    CHECK(umc_heap_allocate(&h,8,&a)==UMC_HEAP_OK);
    CHECK(umc_heap_allocate(&h,8,&b)==UMC_HEAP_OK);
    CHECK(umc_heap_allocate(&h,8,&c)==UMC_HEAP_OK);
    CHECK(umc_heap_set_edge(&h,a,0,b)==UMC_HEAP_OK && umc_heap_set_edge(&h,b,0,a)==UMC_HEAP_OK);
    size_t freed=99;CHECK(umc_heap_collect(&h,&a,1,&freed)==UMC_HEAP_OK && freed==1);
    uint8_t out=99;CHECK(umc_heap_read(&h,c,0,&out,1)==UMC_HEAP_HANDLE && out==99);
    CHECK(umc_heap_collect(&h,NULL,0,&freed)==UMC_HEAP_OK && freed==2);
    CHECK(umc_heap_read(&h,a,0,&out,1)==UMC_HEAP_HANDLE);
    CHECK(umc_heap_read(&h,b,0,&out,1)==UMC_HEAP_HANDLE);
    CHECK(umc_heap_allocate(&h,8,&c)==UMC_HEAP_OK && c!=a);
    CHECK(umc_heap_read(&h,c,0,&out,1)==UMC_HEAP_OK && out==0);
    return 0;
}
static int bounds(void)
{
    UmcManagedHeap h;umc_heap_init(&h);UmcHandle a=99;
    CHECK(umc_heap_allocate(&h,UMC_HEAP_BYTES+1,&a)==UMC_HEAP_RANGE && a==99);
    CHECK(umc_heap_allocate(&h,2,&a)==UMC_HEAP_OK);
    const uint8_t data[]={17,23};uint8_t out[]={0,0};
    CHECK(umc_heap_write(&h,a,0,data,2)==UMC_HEAP_OK);
    CHECK(umc_heap_read(&h,a,0,out,2)==UMC_HEAP_OK && !memcmp(data,out,2));
    UmcManagedHeap before=h;
    CHECK(umc_heap_write(&h,a,1,data,SIZE_MAX)==UMC_HEAP_RANGE && !memcmp(&h,&before,sizeof h));
    CHECK(umc_heap_write(&h,a,SIZE_MAX,data,1)==UMC_HEAP_RANGE);
    CHECK(umc_heap_read(&h,a,2,NULL,0)==UMC_HEAP_OK);
    CHECK(umc_heap_read(&h,a,3,NULL,0)==UMC_HEAP_RANGE);
    CHECK(umc_heap_write(&h,a,0,NULL,1)==UMC_HEAP_ARGUMENT);
    CHECK(umc_heap_set_edge(&h,a,UMC_HEAP_EDGES,0)==UMC_HEAP_RANGE);
    CHECK(umc_heap_set_edge(&h,a,0,UINT64_MAX)==UMC_HEAP_HANDLE);
    return 0;
}
static int atomic(void)
{
    UmcManagedHeap h;umc_heap_init(&h);UmcHandle a=0;
    CHECK(umc_heap_allocate(&h,1,&a)==UMC_HEAP_OK);
    const UmcHandle roots[]={a,UINT64_MAX};UmcManagedHeap before=h;size_t freed=99;
    CHECK(umc_heap_collect(&h,roots,2,&freed)==UMC_HEAP_HANDLE && freed==99 && !memcmp(&h,&before,sizeof h));
    CHECK(umc_heap_collect(&h,NULL,1,&freed)==UMC_HEAP_ARGUMENT && freed==99);
    CHECK(umc_heap_collect(&h,&a,UMC_HEAP_SLOTS+1,&freed)==UMC_HEAP_RANGE && freed==99);
    h.objects[0].edges[0]=UINT64_MAX;before=h;
    CHECK(umc_heap_collect(&h,NULL,0,&freed)==UMC_HEAP_CORRUPT && !memcmp(&h,&before,sizeof h));
    CHECK(umc_heap_allocate(&h,1,&a)==UMC_HEAP_CORRUPT);
    return 0;
}
static int exhausted(void)
{
    UmcManagedHeap h;umc_heap_init(&h);UmcHandle handles[UMC_HEAP_SLOTS];
    for(size_t i=0;i<UMC_HEAP_SLOTS;++i)CHECK(umc_heap_allocate(&h,0,&handles[i])==UMC_HEAP_OK);
    UmcHandle untouched=99;CHECK(umc_heap_allocate(&h,0,&untouched)==UMC_HEAP_FULL && untouched==99);
    size_t freed;CHECK(umc_heap_collect(&h,handles,UMC_HEAP_SLOTS,&freed)==UMC_HEAP_OK && freed==0);
    CHECK(umc_heap_collect(&h,NULL,0,&freed)==UMC_HEAP_OK && freed==UMC_HEAP_SLOTS);
    /* Inject boundary generations in unused slots to qualify retirement without
     * billions of allocations; normal callers never modify heap fields. */
    for(size_t i=0;i<UMC_HEAP_SLOTS;++i)h.objects[i].generation=UINT32_MAX;
    CHECK(umc_heap_allocate(&h,0,&untouched)==UMC_HEAP_FULL && untouched==99);
    h.objects[7].generation=UINT32_MAX-1;
    CHECK(umc_heap_allocate(&h,0,&untouched)==UMC_HEAP_OK && (uint32_t)(untouched>>32)==UINT32_MAX);
    CHECK(umc_heap_collect(&h,NULL,0,&freed)==UMC_HEAP_OK && freed==1);
    CHECK(umc_heap_allocate(&h,0,&untouched)==UMC_HEAP_FULL);
    return 0;
}
static int payload(void)
{
    UmcManagedHeap h;umc_heap_init(&h);UmcHandle a=0,b=0;
    CHECK(umc_heap_allocate(&h,sizeof b,&a)==UMC_HEAP_OK);
    CHECK(umc_heap_allocate(&h,1,&b)==UMC_HEAP_OK);
    CHECK(umc_heap_write(&h,a,0,(const uint8_t *)&b,sizeof b)==UMC_HEAP_OK);
    const UmcHandle roots[]={0,a,a,0};size_t freed=99;
    CHECK(umc_heap_collect(&h,roots,4,&freed)==UMC_HEAP_OK && freed==1);
    CHECK(umc_heap_set_edge(&h,a,0,b)==UMC_HEAP_HANDLE);
    return 0;
}
static int graphs(void)
{
    /* Independent transitive-closure oracle uses a boolean matrix. */
    for(unsigned seed=0;seed<32;++seed) {
        UmcManagedHeap h;umc_heap_init(&h);UmcHandle handles[8];int reachable[8][8]={{0}};
        for(size_t i=0;i<8;++i)CHECK(umc_heap_allocate(&h,1,&handles[i])==UMC_HEAP_OK);
        for(size_t i=0;i<8;++i) {
            reachable[i][i]=1;
            for(size_t e=0;e<UMC_HEAP_EDGES;++e) {
                size_t j=(i*3+e*5+seed)%8;
                if(((seed+i+e)%3)!=0) {
                    CHECK(umc_heap_set_edge(&h,handles[i],e,handles[j])==UMC_HEAP_OK);
                    reachable[i][j]=1;
                }
            }
        }
        for(size_t k=0;k<8;++k)for(size_t i=0;i<8;++i)for(size_t j=0;j<8;++j)
            reachable[i][j]=reachable[i][j]||(reachable[i][k]&&reachable[k][j]);
        size_t root=seed%8,expected=0;
        for(size_t i=0;i<8;++i)if(!reachable[root][i])++expected;
        size_t freed=99;CHECK(umc_heap_collect(&h,&handles[root],1,&freed)==UMC_HEAP_OK && freed==expected);
        for(size_t i=0;i<8;++i) {
            uint8_t value=99;UmcHeapStatus status=umc_heap_read(&h,handles[i],0,&value,1);
            CHECK(status==(reachable[root][i]?UMC_HEAP_OK:UMC_HEAP_HANDLE));
        }
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"cycle"))return cycle();
    if(!strcmp(argv[1],"bounds"))return bounds();
    if(!strcmp(argv[1],"atomic"))return atomic();
    if(!strcmp(argv[1],"exhausted"))return exhausted();
    if(!strcmp(argv[1],"payload"))return payload();
    if(!strcmp(argv[1],"graphs"))return graphs();
    return 2;
}
