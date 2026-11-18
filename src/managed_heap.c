/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/managed_heap.c
 *
 * PURPOSE:
 *   Collect unreachable bounded objects with explicit roots, checked handles and no host allocation.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#include "umicom_c/managed_heap.h"

static int locate(const UmcManagedHeap *heap,UmcHandle handle,size_t *slot)
{
    const uint32_t low=(uint32_t)handle, generation=(uint32_t)(handle>>32);
    if(low==0U || low>UMC_HEAP_SLOTS || generation==0U)return 0;
    const size_t index=(size_t)(low-1U);
    if(heap->objects[index].live!=1U || heap->objects[index].generation!=generation)return 0;
    *slot=index; return 1;
}
static int valid(const UmcManagedHeap *heap)
{
    for(size_t i=0;i<UMC_HEAP_SLOTS;++i) {
        const UmcHeapObject *o=&heap->objects[i];
        if(o->live>1U || o->length>UMC_HEAP_BYTES || (o->live && o->generation==0U) || (!o->live && o->length))return 0;
        for(size_t j=0;j<UMC_HEAP_EDGES;++j) {
            size_t target=0U;
            if(o->edges[j] && (!o->live || !locate(heap,o->edges[j],&target)))return 0;
        }
    }
    return 1;
}
void umc_heap_init(UmcManagedHeap *heap)
{
    if(!heap)return;
    /* Volatile stores keep freestanding initialization independent of libc. */
    volatile unsigned char *bytes=(volatile unsigned char *)heap;
    for(size_t i=0;i<sizeof *heap;++i)bytes[i]=0U;
}
UmcHeapStatus umc_heap_allocate(UmcManagedHeap *heap,size_t bytes,UmcHandle *handle)
{
    if(!heap || !handle)return UMC_HEAP_ARGUMENT;
    if(bytes>UMC_HEAP_BYTES)return UMC_HEAP_RANGE;
    if(!valid(heap))return UMC_HEAP_CORRUPT;
    for(size_t i=0;i<UMC_HEAP_SLOTS;++i) {
        UmcHeapObject *o=&heap->objects[i];
        if(o->live || o->generation==UINT32_MAX)continue;
        ++o->generation; o->length=bytes; o->live=1U;
        for(size_t j=0;j<UMC_HEAP_BYTES;++j)o->bytes[j]=0U;
        *handle=((uint64_t)o->generation<<32)|(uint64_t)(i+1U);
        return UMC_HEAP_OK;
    }
    return UMC_HEAP_FULL;
}
static UmcHeapStatus admit(const UmcManagedHeap *heap,UmcHandle handle,size_t offset,size_t count,size_t *slot)
{
    if(!heap)return UMC_HEAP_ARGUMENT;
    if(!valid(heap))return UMC_HEAP_CORRUPT;
    if(!locate(heap,handle,slot))return UMC_HEAP_HANDLE;
    const size_t length=heap->objects[*slot].length;
    if(offset>length || count>length-offset)return UMC_HEAP_RANGE;
    return UMC_HEAP_OK;
}
UmcHeapStatus umc_heap_write(UmcManagedHeap *heap,UmcHandle handle,size_t offset,const uint8_t *bytes,size_t count)
{
    if(!bytes && count)return UMC_HEAP_ARGUMENT;
    size_t slot=0U; const UmcHeapStatus status=admit(heap,handle,offset,count,&slot);
    if(status!=UMC_HEAP_OK)return status;
    for(size_t i=0;i<count;++i)heap->objects[slot].bytes[offset+i]=bytes[i];
    return UMC_HEAP_OK;
}
UmcHeapStatus umc_heap_read(const UmcManagedHeap *heap,UmcHandle handle,size_t offset,uint8_t *bytes,size_t count)
{
    if(!bytes && count)return UMC_HEAP_ARGUMENT;
    size_t slot=0U; const UmcHeapStatus status=admit(heap,handle,offset,count,&slot);
    if(status!=UMC_HEAP_OK)return status;
    for(size_t i=0;i<count;++i)bytes[i]=heap->objects[slot].bytes[offset+i];
    return UMC_HEAP_OK;
}
UmcHeapStatus umc_heap_set_edge(UmcManagedHeap *heap,UmcHandle owner,size_t edge,UmcHandle target)
{
    if(!heap)return UMC_HEAP_ARGUMENT;
    if(edge>=UMC_HEAP_EDGES)return UMC_HEAP_RANGE;
    if(!valid(heap))return UMC_HEAP_CORRUPT;
    size_t from=0U,to=0U;
    if(!locate(heap,owner,&from) || (target && !locate(heap,target,&to)))return UMC_HEAP_HANDLE;
    heap->objects[from].edges[edge]=target;
    return UMC_HEAP_OK;
}
/* Share the existing bounded mark walk with read-only pressure inspection so both operations agree on exact roots, cycles and invalid handles. The former implementation is retained for owner review. */
#if 0
UmcHeapStatus umc_heap_collect(UmcManagedHeap *heap,const UmcHandle *roots,size_t count,size_t *collected)
{
    if(!heap || !collected || (!roots && count))return UMC_HEAP_ARGUMENT;
    if(count>UMC_HEAP_SLOTS)return UMC_HEAP_RANGE;
    if(!valid(heap))return UMC_HEAP_CORRUPT;
    uint32_t marked=0U;
    size_t pending[UMC_HEAP_SLOTS],used=0U;
    for(size_t i=0;i<count;++i) {
        size_t slot=0U;
        if(!roots[i])continue;
        if(!locate(heap,roots[i],&slot))return UMC_HEAP_HANDLE;
        const uint32_t bit=UINT32_C(1)<<(unsigned)slot;
        if(!(marked&bit)){marked|=bit;pending[used++]=slot;}
    }
    /* Mark when enqueued, not when removed: each slot enters once, so even
     * cycles and duplicate roots cannot overrun the fixed traversal stack. */
    while(used) {
        const UmcHeapObject *o=&heap->objects[pending[--used]];
        for(size_t i=0;i<UMC_HEAP_EDGES;++i) {
            if(!o->edges[i])continue;
            size_t slot=0U;
            (void)locate(heap,o->edges[i],&slot); /* Complete graph admitted above. */
            const uint32_t bit=UINT32_C(1)<<(unsigned)slot;
            if(!(marked&bit)){marked|=bit;pending[used++]=slot;}
        }
    }
    size_t freed=0U;
    for(size_t i=0;i<UMC_HEAP_SLOTS;++i) {
        UmcHeapObject *o=&heap->objects[i];
        if(!o->live || (marked&(UINT32_C(1)<<(unsigned)i)))continue;
        o->live=0U;o->length=0U;
        for(size_t j=0;j<UMC_HEAP_EDGES;++j)o->edges[j]=0U;
        for(size_t j=0;j<UMC_HEAP_BYTES;++j)o->bytes[j]=0U;
        ++freed;
    }
    *collected=freed; return UMC_HEAP_OK;
}

#endif
static UmcHeapStatus reachable(const UmcManagedHeap *heap,const UmcHandle *roots,size_t count,uint32_t *bits)
{
    if(!heap || !bits || (!roots && count))return UMC_HEAP_ARGUMENT;
    if(count>UMC_HEAP_SLOTS)return UMC_HEAP_RANGE;
    if(!valid(heap))return UMC_HEAP_CORRUPT;
    uint32_t marked=0U;
    size_t pending[UMC_HEAP_SLOTS],used=0U;
    for(size_t i=0;i<count;++i) {
        size_t slot=0U;
        if(!roots[i])continue;
        if(!locate(heap,roots[i],&slot))return UMC_HEAP_HANDLE;
        const uint32_t bit=UINT32_C(1)<<(unsigned)slot;
        if(!(marked&bit)){marked|=bit;pending[used++]=slot;}
    }
    /* Mark when enqueued, not when removed: each slot enters once, so even
     * cycles and duplicate roots cannot overrun the fixed traversal stack. */
    while(used) {
        const UmcHeapObject *o=&heap->objects[pending[--used]];
        for(size_t i=0;i<UMC_HEAP_EDGES;++i) {
            if(!o->edges[i])continue;
            size_t slot=0U;
            (void)locate(heap,o->edges[i],&slot); /* Complete graph admitted above. */
            const uint32_t bit=UINT32_C(1)<<(unsigned)slot;
            if(!(marked&bit)){marked|=bit;pending[used++]=slot;}
        }
    }
    *bits=marked; return UMC_HEAP_OK;
}

/* Inspection and collection share graph admission and marking. A caller can
 * explain pressure without changing generations, edges or payloads. The report
 * belongs to this exact serialized snapshot and root set, not a later mutation. */
UmcHeapStatus umc_heap_inspect(const UmcManagedHeap *heap,const UmcHandle *roots,
    size_t count,UmcHeapReport *report)
{
    if(!report)return UMC_HEAP_ARGUMENT;
    uint32_t marked=0U;
    const UmcHeapStatus status=reachable(heap,roots,count,&marked);
    if(status!=UMC_HEAP_OK)return status;
/* Initialize every report byte explicitly so unoptimized freestanding targets need no implicit memset. The former implementation is retained for owner review. */
#if 0
    UmcHeapReport next={0};
#endif
    UmcHeapReport next;
    volatile unsigned char *initial=(volatile unsigned char *)&next;
    for(size_t i=0;i<sizeof next;++i)initial[i]=0U;
    next.arena_bytes=sizeof *heap;
    for(size_t i=0;i<UMC_HEAP_SLOTS;++i) {
        const UmcHeapObject *o=&heap->objects[i];
        if(!o->live) {
            if(o->generation==UINT32_MAX)++next.retired_slots;
            else ++next.reusable_slots;
            continue;
        }
        ++next.live_objects;
        next.live_payload_bytes+=o->length;
        for(size_t e=0;e<UMC_HEAP_EDGES;++e)
            if(o->edges[e])++next.live_edges;
        if(marked&(UINT32_C(1)<<(unsigned)i)) {
            ++next.reachable_objects;
            next.reachable_payload_bytes+=o->length;
        } else {
            ++next.collectable_objects;
            next.collectable_payload_bytes+=o->length;
        }
    }
/* Publish the complete admitted report with bounded byte stores so freestanding builds need no implicit memcpy. The former implementation is retained for owner review. */
#if 0
    *report=next;
#endif
    const volatile unsigned char *source=(const volatile unsigned char *)&next;
    volatile unsigned char *destination=(volatile unsigned char *)report;
    for(size_t i=0;i<sizeof next;++i)destination[i]=source[i];
    return UMC_HEAP_OK;
}

UmcHeapStatus umc_heap_collect(UmcManagedHeap *heap,const UmcHandle *roots,size_t count,size_t *collected)
{
    if(!collected)return UMC_HEAP_ARGUMENT;
    uint32_t marked=0U;
    const UmcHeapStatus status=reachable(heap,roots,count,&marked);
    if(status!=UMC_HEAP_OK)return status;
    size_t freed=0U;
    for(size_t i=0;i<UMC_HEAP_SLOTS;++i) {
        UmcHeapObject *o=&heap->objects[i];
        if(!o->live || (marked&(UINT32_C(1)<<(unsigned)i)))continue;
        o->live=0U;o->length=0U;
        for(size_t j=0;j<UMC_HEAP_EDGES;++j)o->edges[j]=0U;
        for(size_t j=0;j<UMC_HEAP_BYTES;++j)o->bytes[j]=0U;
        ++freed;
    }
    *collected=freed; return UMC_HEAP_OK;
}
