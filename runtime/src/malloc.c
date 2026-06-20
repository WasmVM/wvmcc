// M2-11: minimum allocator — bump from __heap_base, no-op free.
//
// __heap_base is imported by the linker's crt0. We track a running
// offset and hand out aligned slabs. No bound checking, no
// `memory.grow` (wvmcc doesn't expose it as an intrinsic yet); if the
// process actually runs out of linear memory, the next store traps.
// Real allocator deferred to a follow-up milestone.

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <limits.h>

#ifndef SIZE_MAX
#define SIZE_MAX ULONG_MAX
#endif

// __heap_base is imported as a Wasm global (i64) holding the address
// where the linker placed the start of the heap. We read its value
// directly — *not* its address (Wasm globals don't live in linear
// memory). The running offset is kept in another file-scope long.
extern unsigned long __heap_base;
static unsigned long __heap_offset = 0;

void *malloc(size_t size) {
    if (size == 0) return (void *)0;
    /* 8-byte align */
    size = (size + 7) & ~(size_t)7;
    unsigned long p = __heap_base + __heap_offset;
    __heap_offset += size;
    return (void *)p;
}

void *aligned_alloc(size_t alignment, size_t size) {
    if (alignment == 0) return (void *)0;
    /* 7.22.3.1: the requested alignment must be supported; for the bump
       allocator we simply advance the running offset so the next slab's
       in-memory address is a multiple of `alignment`. (The pointer's low
       bits carry the mem offset; the high-nibble tag is untouched by the
       modulo.) */
    unsigned long base = __heap_base + __heap_offset;
    unsigned long misalign = base % alignment;
    if (misalign) __heap_offset += (alignment - misalign);
    return malloc(size);
}

void free(void *ptr) {
    /* No-op: bump allocator never reclaims. Memory is freed at process
       exit. This is a documented M2 limitation. */
    (void)ptr;
}

void *calloc(size_t n, size_t sz) {
    if (n != 0 && sz > SIZE_MAX / n) {
        errno = ENOMEM;
        return (void *)0;
    }
    size_t total = n * sz;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == (void *)0) return malloc(size);
    if (size == 0) { free(ptr); return (void *)0; }
    void *np = malloc(size);
    if (!np) return (void *)0;
    /* No per-block size metadata in a bump allocator — copy `size`
       bytes from the old block. For shrinks this matches realloc's
       contract; for grows the tail is garbage but the front bytes
       (up to min(old, new)) are preserved, which is what callers rely
       on. */
    memcpy(np, ptr, size);
    return np;
}
