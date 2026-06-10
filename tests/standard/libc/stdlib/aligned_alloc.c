/* tests/standard/libc/stdlib/aligned_alloc.c — LIBC-stdlib-aligned_alloc-01
 * (C17 7.22.3.1). aligned_alloc allocates with the requested alignment
 * (size a multiple of the alignment). Verify=exit. */
#include <stdlib.h>

int main(void) {
    void *p = aligned_alloc(16, 32);
    if (!p) return 1;
    /* The low bits of the tagged pointer are the in-memory offset. */
    if ((unsigned long long)p % 16 != 0) return 2;
    *(char *)p = 7;
    if (*(char *)p != 7) return 3;
    free(p);
    return 0;
}
