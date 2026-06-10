/* tests/standard/libc/stdlib/qsort.c — LIBC-stdlib-qsort-01 (C17 7.22.5.2).
 * qsort sorts an array into ascending order per the comparator. Verify=exit. */
#include <stdlib.h>

static int cmp_int(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

int main(void) {
    int a[5] = {3, 1, 2, 5, 4};
    qsort(a, 5, sizeof(int), cmp_int);
    for (int i = 0; i < 5; ++i) {
        if (a[i] != i + 1) return i + 1;
    }
    return 0;
}
