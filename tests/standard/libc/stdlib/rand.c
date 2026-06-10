/* tests/standard/libc/stdlib/rand.c — LIBC-stdlib-rand-01 (C17 7.22.2.1-2).
 * rand returns values in [0, RAND_MAX]; srand makes the sequence
 * reproducible for the same seed. Verify=exit. */
#include <stdlib.h>

int main(void) {
    srand(7u);
    int a = rand();
    if (a < 0 || a > RAND_MAX) return 1;
    int b = rand();
    if (b < 0 || b > RAND_MAX) return 2;
    srand(7u);                  /* same seed -> same sequence (7.22.2.2p2) */
    if (rand() != a) return 3;
    if (rand() != b) return 4;
    return 0;
}
