/* tests/standard/libc/stdlib/calloc.c — LIBC-stdlib-calloc-01 (C17 7.22.3.2).
 * calloc allocates an array and zero-initializes every byte. Verify=exit. */
#include <stdlib.h>

int main(void) {
    unsigned char *p = calloc(4, 2);
    if (!p) return 1;
    for (int i = 0; i < 8; ++i) {
        if (p[i] != 0) return 2;
    }
    p[3] = 7; /* storage is writable */
    if (p[3] != 7) return 3;
    free(p);
    return 0;
}
