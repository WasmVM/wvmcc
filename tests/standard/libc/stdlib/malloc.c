/* tests/standard/libc/stdlib/malloc.c — LIBC-stdlib-malloc-01 (C17 7.22.3.4).
 * malloc allocates writable, suitably aligned storage. Verify=exit. */
#include <stdlib.h>

int main(void) {
    unsigned char *p = malloc(8);
    if (!p) return 1;
    p[0] = 0x5a;
    p[7] = 0xa5;
    if (p[0] != 0x5a || p[7] != 0xa5) return 2;
    /* 7.22.3p1: aligned for any object with fundamental alignment. The
     * high-nibble pointer tag does not disturb the low bits. */
    if ((unsigned long long)p % _Alignof(long long) != 0) return 3;
    free(p);
    return 0;
}
