/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/managed_cycle.c
 *
 * PURPOSE:
 *   Demonstrate explicit roots, cyclic reachability and stale handle refusal in a bounded arena.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/


#include "umicom_c/managed_heap.h"
#include <stdio.h>
/* Storage belongs to the caller. This example avoids both malloc and a large
 * automatic frame; a second concurrent request would need its own arena. */
static UmcManagedHeap heap;
int main(void)
{
    UmcHandle first=0,second=0;
    size_t collected=99;
    const uint8_t message[]={ 'O','K' };
    uint8_t output[2]={0};
    umc_heap_init(&heap);
    if(umc_heap_allocate(&heap,sizeof message,&first)!=UMC_HEAP_OK ||
       umc_heap_allocate(&heap,0,&second)!=UMC_HEAP_OK ||
       umc_heap_write(&heap,first,0,message,sizeof message)!=UMC_HEAP_OK ||
       umc_heap_set_edge(&heap,first,0,second)!=UMC_HEAP_OK ||
       umc_heap_set_edge(&heap,second,0,first)!=UMC_HEAP_OK)return 1;
    /* Report reachability without changing the graph. Logical payload length
     * is separate from the fixed arena that remains allocated after collection. */
    UmcHeapReport report;
    if(umc_heap_inspect(&heap,&first,1,&report)!=UMC_HEAP_OK ||
       report.reachable_objects!=2 || report.collectable_objects!=0 ||
       report.live_payload_bytes!=sizeof message)return 1;
    printf("Inspection: %zu live, %zu reachable, %zu collectable, %zu payload bytes\n",
           report.live_objects,report.reachable_objects,report.collectable_objects,report.live_payload_bytes);
    if(umc_heap_inspect(&heap,NULL,0,&report)!=UMC_HEAP_OK || report.collectable_objects!=2)return 1;
    puts("Inspection with no roots: 2 collectable; heap unchanged");
    /* The local variable is not discovered automatically: pass its handle as
     * a root. Both objects survive because each is reachable from that root. */
    if(umc_heap_collect(&heap,&first,1,&collected)!=UMC_HEAP_OK || collected!=0 ||
       umc_heap_read(&heap,first,0,output,sizeof output)!=UMC_HEAP_OK)return 1;
    if(output[0]!='O'||output[1]!='K')return 1;
    printf("Rooted cycle retained: %c%c\n",(int)output[0],(int)output[1]);
    /* An empty root set makes the cycle unreachable despite its internal edges. */
    if(umc_heap_collect(&heap,NULL,0,&collected)!=UMC_HEAP_OK || collected!=2)return 1;
    if(umc_heap_read(&heap,first,0,output,sizeof output)!=UMC_HEAP_HANDLE)return 1;
    puts("Unrooted cycle collected: 2; old handle refused");
    if(umc_heap_inspect(&heap,NULL,0,&report)!=UMC_HEAP_OK ||
       report.live_objects!=0 || report.reusable_slots!=UMC_HEAP_SLOTS ||
       report.arena_bytes!=sizeof heap)return 1;
    printf("After collection: %zu reusable slots; arena storage unchanged\n",report.reusable_slots);
    return 0;
}
