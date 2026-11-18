/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_heap_inspection.c
 *
 * PURPOSE:
 *   Verify read-only graph reports, failed-operation preservation and generation retirement.
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
static int totals(void) {
    UmcManagedHeap h;umc_heap_init(&h);UmcHeapReport r;
    CHECK(umc_heap_inspect(&h,NULL,0,&r)==UMC_HEAP_OK);
    CHECK(r.arena_bytes==sizeof h && r.reusable_slots==32 && r.live_objects==0);
    CHECK(r.live_edges==0 && r.live_payload_bytes==0 && r.retired_slots==0);
    UmcHandle handles[32];
    for(size_t i=0;i<32;++i)CHECK(umc_heap_allocate(&h,128,&handles[i])==UMC_HEAP_OK);
    UmcManagedHeap before=h;
    CHECK(umc_heap_inspect(&h,handles,32,&r)==UMC_HEAP_OK && !memcmp(&h,&before,sizeof h));
    CHECK(r.live_objects==32 && r.reachable_objects==32 && r.reachable_payload_bytes==4096);
    CHECK(r.live_payload_bytes==4096 && r.collectable_objects==0 && r.collectable_payload_bytes==0 && r.reusable_slots==0);
    CHECK(umc_heap_inspect(&h,NULL,0,&r)==UMC_HEAP_OK && !memcmp(&h,&before,sizeof h));
    CHECK(r.reachable_objects==0 && r.reachable_payload_bytes==0 && r.collectable_objects==32 && r.collectable_payload_bytes==4096);
    return 0;
}
static int cycles(void) {
    UmcManagedHeap h;umc_heap_init(&h);UmcHandle a,b,c,d;
    CHECK(umc_heap_allocate(&h,8,&a)==UMC_HEAP_OK);
    CHECK(umc_heap_allocate(&h,16,&b)==UMC_HEAP_OK);
    CHECK(umc_heap_allocate(&h,32,&c)==UMC_HEAP_OK);
    CHECK(umc_heap_allocate(&h,0,&d)==UMC_HEAP_OK);
    CHECK(umc_heap_set_edge(&h,a,0,b)==UMC_HEAP_OK);
    CHECK(umc_heap_set_edge(&h,b,0,a)==UMC_HEAP_OK);
    CHECK(umc_heap_set_edge(&h,c,0,c)==UMC_HEAP_OK);
    /* Payload bytes are data, even when they encode a valid handle. */
    CHECK(umc_heap_write(&h,a,0,(const uint8_t *)&c,sizeof c)==UMC_HEAP_OK);
    const UmcHandle roots[]={0,a,a,d};UmcHeapReport r;UmcManagedHeap before=h;
    CHECK(umc_heap_inspect(&h,roots,4,&r)==UMC_HEAP_OK && !memcmp(&h,&before,sizeof h));
    CHECK(r.live_objects==4 && r.live_payload_bytes==56 && r.live_edges==3);
    CHECK(r.reachable_objects==3 && r.reachable_payload_bytes==24);
    CHECK(r.collectable_objects==1 && r.collectable_payload_bytes==32 && r.reusable_slots==28);
    size_t freed;CHECK(umc_heap_collect(&h,roots,4,&freed)==UMC_HEAP_OK && freed==r.collectable_objects);
    CHECK(umc_heap_inspect(&h,roots,4,&r)==UMC_HEAP_OK);
    CHECK(r.live_objects==3 && r.live_edges==2 && r.reusable_slots==29 && r.collectable_objects==0 && r.arena_bytes==sizeof h);
    return 0;
}
static int refusal(void) {
    UmcManagedHeap h;umc_heap_init(&h);UmcHandle a;UmcHeapReport r,old;
    CHECK(umc_heap_allocate(&h,1,&a)==UMC_HEAP_OK);
    memset(&r,0xa5,sizeof r);memcpy(&old,&r,sizeof r);UmcManagedHeap before=h;
    CHECK(umc_heap_inspect(NULL,NULL,0,&r)==UMC_HEAP_ARGUMENT);
    CHECK(umc_heap_inspect(&h,NULL,0,NULL)==UMC_HEAP_ARGUMENT);
    CHECK(umc_heap_inspect(&h,NULL,1,&r)==UMC_HEAP_ARGUMENT);
    CHECK(umc_heap_inspect(&h,&a,33,&r)==UMC_HEAP_RANGE);
    const UmcHandle roots[]={a,UINT64_MAX};
    CHECK(umc_heap_inspect(&h,roots,2,&r)==UMC_HEAP_HANDLE);
    CHECK(!memcmp(&r,&old,sizeof r) && !memcmp(&h,&before,sizeof h));
    size_t freed;CHECK(umc_heap_collect(&h,NULL,0,&freed)==UMC_HEAP_OK);before=h;
    CHECK(umc_heap_inspect(&h,&a,1,&r)==UMC_HEAP_HANDLE);
    CHECK(!memcmp(&r,&old,sizeof r) && !memcmp(&h,&before,sizeof h));
    /* Only tests inject invalid metadata; normal consumers use API functions. */
    h.objects[7].edges[0]=a;before=h;
    CHECK(umc_heap_inspect(&h,NULL,0,&r)==UMC_HEAP_CORRUPT);
    CHECK(!memcmp(&r,&old,sizeof r) && !memcmp(&h,&before,sizeof h));
    return 0;
}
static int retirement(void) {
    UmcManagedHeap h;umc_heap_init(&h);UmcHeapReport r;
    /* Reach the generation boundary without billions of test allocations. */
    for(size_t i=0;i<32;++i)h.objects[i].generation=UINT32_MAX;
    h.objects[9].generation=UINT32_MAX-1U;
    CHECK(umc_heap_inspect(&h,NULL,0,&r)==UMC_HEAP_OK && r.retired_slots==31 && r.reusable_slots==1);
    UmcHandle a;CHECK(umc_heap_allocate(&h,0,&a)==UMC_HEAP_OK);
    CHECK(umc_heap_inspect(&h,&a,1,&r)==UMC_HEAP_OK);
    CHECK(r.retired_slots==31 && r.reusable_slots==0 && r.live_objects==1 && r.reachable_objects==1 && r.live_payload_bytes==0);
    size_t freed;CHECK(umc_heap_collect(&h,NULL,0,&freed)==UMC_HEAP_OK && freed==1);
    CHECK(umc_heap_inspect(&h,NULL,0,&r)==UMC_HEAP_OK);
    CHECK(r.retired_slots==32 && r.reusable_slots==0 && r.live_objects==0 && r.arena_bytes==sizeof h);
    return 0;
}
static int graphs(void) {
    /* Independent matrix closure avoids repeating the production mark walk. */
    for(size_t seed=0;seed<64;++seed) {
        UmcManagedHeap h;umc_heap_init(&h);UmcHandle handles[8];int reach[8][8]={{0}};size_t edges=0;
        for(size_t i=0;i<8;++i){CHECK(umc_heap_allocate(&h,i,&handles[i])==UMC_HEAP_OK);reach[i][i]=1;}
        for(size_t i=0;i<8;++i)for(size_t e=0;e<4;++e) {
            const size_t j=(seed+i*3+e*5)%8;if((seed+i+e)%4==0)continue;
            CHECK(umc_heap_set_edge(&h,handles[i],e,handles[j])==UMC_HEAP_OK);reach[i][j]=1;++edges;
        }
        for(size_t k=0;k<8;++k)for(size_t i=0;i<8;++i)for(size_t j=0;j<8;++j)
            reach[i][j]=reach[i][j]||(reach[i][k]&&reach[k][j]);
        const size_t a=seed%8,b=(seed/8)%8;const UmcHandle roots[]={handles[a],0,handles[b],handles[a]};
        size_t objects=0,bytes=0;
        for(size_t i=0;i<8;++i)if(reach[a][i]||reach[b][i]){++objects;bytes+=i;}
        UmcManagedHeap before=h;UmcHeapReport r;
        CHECK(umc_heap_inspect(&h,roots,4,&r)==UMC_HEAP_OK && !memcmp(&h,&before,sizeof h));
        CHECK(r.reachable_objects==objects && r.reachable_payload_bytes==bytes);
        CHECK(r.collectable_objects==8-objects && r.collectable_payload_bytes==28-bytes);
        CHECK(r.live_edges==edges && r.live_objects==8 && r.live_payload_bytes==28 && r.reusable_slots==24 && r.retired_slots==0);
        size_t freed;CHECK(umc_heap_collect(&h,roots,4,&freed)==UMC_HEAP_OK && freed==8-objects);
    }
    return 0;
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"totals"))return totals();
    if(!strcmp(argv[1],"cycles"))return cycles();
    if(!strcmp(argv[1],"refusal"))return refusal();
    if(!strcmp(argv[1],"retirement"))return retirement();
    if(!strcmp(argv[1],"graphs"))return graphs();
    return 2;
}
