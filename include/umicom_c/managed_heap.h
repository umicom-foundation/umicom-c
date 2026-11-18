/*-----------------------------------------------------------------------------
 * Umicom C
 * File: include/umicom_c/managed_heap.h
 *
 * PURPOSE:
 *   Define an optional bounded handle heap with explicit roots and cycle collection.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

#ifndef UMICOM_C_MANAGED_HEAP_H
#define UMICOM_C_MANAGED_HEAP_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define UMC_HEAP_SLOTS 32U
#define UMC_HEAP_BYTES 128U
#define UMC_HEAP_EDGES 4U
typedef uint64_t UmcHandle; /* zero is null; values are arena-local, not pointers. */
typedef enum UmcHeapStatus {
    UMC_HEAP_OK, UMC_HEAP_ARGUMENT, UMC_HEAP_HANDLE, UMC_HEAP_FULL,
    UMC_HEAP_RANGE, UMC_HEAP_CORRUPT
} UmcHeapStatus;
typedef struct UmcHeapObject {
    uint32_t generation;
    unsigned live;
    size_t length;
    UmcHandle edges[UMC_HEAP_EDGES];
    uint8_t bytes[UMC_HEAP_BYTES];
} UmcHeapObject;
typedef struct UmcManagedHeap {
    UmcHeapObject objects[UMC_HEAP_SLOTS];
} UmcManagedHeap;
/* Counts describe one admitted graph and the supplied exact root set. Payload
 * bytes are requested logical lengths, not reserved memory. arena_bytes is
 * sizeof(UmcManagedHeap), including storage and metadata; it excludes traversal
 * stack, caller roots, program code and all other process memory. Collection
 * does not shrink that arena. An unused generation-exhausted slot is retired. */
typedef struct UmcHeapReport {
    size_t arena_bytes;
    size_t live_objects, live_payload_bytes, live_edges;
    size_t reachable_objects, reachable_payload_bytes;
    size_t collectable_objects, collectable_payload_bytes;
    size_t reusable_slots, retired_slots;
} UmcHeapReport;

/* Initialize a new lifetime before use. Drop every old handle before reinit.
 * One serialized owner; fields are inspectable but not directly mutable.
 * All output/input regions must be valid, disjoint from the heap and one another.
 * No malloc, host pointers, finalizers, weak references or background threads.
 * This optional library does not change native C malloc/free or VM opcodes. */
void umc_heap_init(UmcManagedHeap *heap);
UmcHeapStatus umc_heap_allocate(UmcManagedHeap *heap,size_t bytes,UmcHandle *handle);
UmcHeapStatus umc_heap_write(UmcManagedHeap *heap,UmcHandle handle,size_t offset,const uint8_t *bytes,size_t count);
UmcHeapStatus umc_heap_read(const UmcManagedHeap *heap,UmcHandle handle,size_t offset,uint8_t *bytes,size_t count);
UmcHeapStatus umc_heap_set_edge(UmcManagedHeap *heap,UmcHandle owner,size_t edge,UmcHandle target);
/* Roots and graph references are exact handles. Byte payloads are never scanned
 * for pointer-looking integers. Supply all roots, including paused VM values,
 * before collection. At most 32 root entries, null allowed. Collection validates
 * the complete graph first; any invalid root/edge leaves heap and collected
 * unchanged. Successful collection clears all unreachable payloads and edges.
 * Reused slots advance generation; exhausted generations are never reused. */
UmcHeapStatus umc_heap_collect(UmcManagedHeap *heap,const UmcHandle *roots,size_t count,size_t *collected);
/* Read-only collection preview; no allocations or mutations. Root admission
 * matches collect. Every failure preserves report and heap. Output is disjoint
 * from heap and roots. Null roots with zero count means no external roots, not
 * automatic discovery. Zero-length objects still occupy slots. Recompute after
 * mutation; never use this report as a substitute for supplying complete roots. */
UmcHeapStatus umc_heap_inspect(const UmcManagedHeap *heap,const UmcHandle *roots,
    size_t count,UmcHeapReport *report);
#ifdef __cplusplus
}
#endif
#endif
