![Umicom](assets/branding/umicom-logo.svg)

# Managed objects and explicit roots

A garbage collector reclaims objects that the program can no longer reach. The optional **UmicomC::ManagedHeap** library demonstrates bounded mark-and-sweep collection. It is separate from ordinary C allocation and the current integer bytecode instruction set.

## Storage and handles

The caller owns one **UmcManagedHeap** containing 32 object slots. Each object has up to 128 payload bytes and four explicit outgoing references. There is no malloc call, background thread or hidden expansion. Allocation returns a 64-bit handle, not a native pointer. Zero means no object.

A handle combines slot identity and generation. Reusing a slot changes its generation, so an earlier handle is refused instead of accessing the replacement object. A slot whose generation cannot advance is retired. Reinitialization begins a new lifetime: discard every old handle first. Handles from different arenas must never be mixed; they are not globally authenticated identities.

Use **umc_heap_write** and **umc_heap_read** with a byte offset and length. Both check the object's declared size. Use **umc_heap_set_edge** to record another handle as a reference. Payload bytes that resemble a handle are still data.

## Follow a cycle

Read [the complete C example](../examples/managed_cycle.c). It allocates two objects, connects each to the other, and writes O and K into the first.

1. Initialize the arena. Its storage must remain valid throughout all operations.
2. Allocate two objects and check every returned status.
3. Set their outgoing edges to form a cycle.
4. Pass the first handle as a root to **umc_heap_collect**. A root is an explicit starting point for reachability. Both objects survive.
5. Read the first payload. The bytes remain O and K.
6. Collect with no roots. Both objects are unreachable despite referencing each other. Both are cleared and freed.
7. Read using the old handle. The operation is refused.

From the configured Umicom C repository:

~~~powershell
cmake --build build/debug --target umicom-c-managed-cycle --parallel 2
ctest --test-dir build/debug -R "umicom.c.managed-heap" --output-on-failure --no-tests=error
~~~

<!-- Earlier output description retained for review: The final example output is **Unrooted cycle collected: 2; old handle refused**. -->
The example still prints **Unrooted cycle collected: 2; old handle refused** and now finishes with **After collection: 32 reusable slots; arena storage unchanged**. The source is compiled by the host C compiler. It is not a heap program executed by the integer VM.

## Ownership rules

Collection first validates every live object's references and every root. Invalid graph/root input leaves the heap and collected-count output unchanged. Success marks reachable objects, then clears unreachable payloads and edges.

Supply all roots explicitly, at most 32 entries. A handle held only in an unlisted C variable does not keep an object alive. A future VM object language needs typed values and precise root enumeration for paused frames before using this collector. That language integration is not implemented.

Keep input/output spans valid and disjoint from the arena. Serialize access. There are no finalizers, weak references, native pointer tracing, moving objects or resource-closing callbacks. Memory collection does not close operating-system files or GPU resources; those need separate ownership.

Before increasing capacity, measure arena size, collection time and target memory limits. Avoid putting an arena on a small Kernel stack. Extend graph-oracle, stale-handle and failure-preservation tests. Collection can reduce some lifetime mistakes; it does not make all native C memory-safe.

## Inspect without collecting

Call **umc_heap_inspect** with the arena, a complete root array, its count and a separate **UmcHeapReport**. A successful call does not change the heap, advance generations or clear data. Invalid arguments, roots or graph metadata leave the report unchanged. A report describes only that serialized snapshot; recompute it after changing roots, edges or objects.

~~~c
UmcHeapReport report;
if (umc_heap_inspect(&heap, &first, 1, &report) == UMC_HEAP_OK) {
    printf("%zu reachable; %zu collectable\n",
           report.reachable_objects, report.collectable_objects);
}
~~~

In the example, the two-object cycle has two reachable objects and none collectable when **first** is a root. With no roots, inspection reports two collectable objects but still leaves them live. The following real collection with **first** as a root retains them, demonstrating that the preview did not collect anything.

| Field | Meaning |
|---|---|
| live_objects | Occupied slots, including zero-length objects |
| live_payload_bytes | Sum of requested lengths, not all reserved storage |
| live_edges | Non-null reference slots, including repeated targets |
| reachable_objects / reachable_payload_bytes | Live objects and logical bytes reachable from the supplied roots |
| collectable_objects / collectable_payload_bytes | Remaining live objects and logical bytes |
| reusable_slots | Currently unused slots whose generation can still advance |
| retired_slots | Currently unused slots whose generation is exhausted |
| arena_bytes | sizeof(UmcManagedHeap), including fixed storage and metadata |

The live, reusable and retired slot counts total 32. Reachable plus collectable counts equal live objects. A live object at the final generation is still live; it becomes retired only after collection. Do not interpret collectable_objects as a promise that all those slots will be reusable afterwards.

**arena_bytes does not shrink after collection.** The caller still owns the same fixed arena. It excludes the traversal stack, root array, program, native allocations and all other process memory. It is not a process memory meter. Inspection uses bounded local working storage and no host allocation. The read-only qualifier does not add synchronization: another thread must not mutate the arena during inspection.

Run the additional report scenarios with **ctest --test-dir build/debug -R "umicom.c.heap-inspection" --output-on-failure --no-tests=error** after building. They cover known graphs, independent transitive closure, null and duplicate roots, invalid handles, zero-length objects and exhausted generations.
