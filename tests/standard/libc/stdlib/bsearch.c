/* tests/standard/libc/stdlib/bsearch.c — LIBC-stdlib-bsearch-01 (C17 7.22.5.1).
 * bsearch finds a matching element in a sorted array, or returns NULL when no
 * element matches. Verify=exit. */
#include <stdlib.h>

static int cmp_int(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

int main(void) {
    int arr[5] = {2, 4, 6, 8, 10};
    int key = 8;
    int *p = bsearch(&key, arr, 5, sizeof(int), cmp_int);
    if (p != &arr[3]) return 1;
    key = 2; /* first element */
    p = bsearch(&key, arr, 5, sizeof(int), cmp_int);
    if (p != &arr[0]) return 2;
    key = 5; /* absent */
    if (bsearch(&key, arr, 5, sizeof(int), cmp_int) != NULL) return 3;
    return 0;
}
