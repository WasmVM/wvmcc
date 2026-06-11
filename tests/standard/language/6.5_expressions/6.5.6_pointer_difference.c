/* LANG-6.5.6-08 — 6.5.6p9 (ISO C17): subtracting two pointers to elements of the
 * same array object yields the difference of the subscripts of the two array
 * elements; the result has type ptrdiff_t. Verify=exit. */
#include <stddef.h>
int main(void) {
    int a[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    if ((&a[5] - &a[2]) != 3) return 1;
    if ((&a[2] - &a[5]) != -3) return 2;   /* signed result */
    if ((&a[0] - &a[0]) != 0) return 3;

    /* (p+i) - p == i for any valid i. */
    int *p = a;
    if (((p + 6) - p) != 6) return 4;

    /* one-past-end minus base equals the array length. */
    if ((&a[8] - a) != 8) return 5;

    /* the difference is a ptrdiff_t value. */
    ptrdiff_t d = &a[7] - &a[1];
    if (d != 6) return 6;

    return 0;
}
