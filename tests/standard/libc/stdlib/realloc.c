/* tests/standard/libc/stdlib/realloc.c — LIBC-stdlib-realloc-01 (C17 7.22.3.5).
 * realloc resizes, preserving contents up to the lesser of the old and new
 * sizes. Verify=exit. */
#include <stdlib.h>

int main(void) {
    unsigned char *p = malloc(4);
    if (!p) return 1;
    p[0] = 1; p[1] = 2; p[2] = 3; p[3] = 4;
    unsigned char *q = realloc(p, 8);   /* grow */
    if (!q) return 2;
    if (q[0] != 1 || q[1] != 2 || q[2] != 3 || q[3] != 4) return 3;
    q[7] = 9;                           /* new tail is writable */
    if (q[7] != 9) return 4;
    unsigned char *r = realloc(q, 2);   /* shrink */
    if (!r) return 5;
    if (r[0] != 1 || r[1] != 2) return 6;
    free(r);
    return 0;
}
