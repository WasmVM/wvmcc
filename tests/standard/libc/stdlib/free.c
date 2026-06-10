/* tests/standard/libc/stdlib/free.c — LIBC-stdlib-free-01 (C17 7.22.3.3).
 * free deallocates; free(NULL) is a no-op. Verify=exit. */
#include <stdlib.h>

int main(void) {
    free(NULL);              /* 7.22.3.3p2: no action occurs */
    void *p = malloc(4);
    if (!p) return 1;
    free(p);
    void *q = malloc(4);     /* allocator remains functional after free */
    if (!q) return 2;
    free(q);
    return 0;
}
