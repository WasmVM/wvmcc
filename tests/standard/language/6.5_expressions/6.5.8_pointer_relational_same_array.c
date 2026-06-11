/* LANG-6.5.8-02 — 6.5.8p5: when two pointers are compared, the result depends
 * on the relative locations in the address space of the objects pointed to.
 * Pointers to elements of the same array object compare consistently with the
 * ordering of their subscripts (a pointer one past the last element compares
 * greater than every element pointer). */

int main(void)
{
    int a[5] = { 0, 1, 2, 3, 4 };
    int *p0 = &a[0];
    int *p2 = &a[2];
    int *p4 = &a[4];
    int *pend = &a[5];   /* one past the last element: valid for comparison */

    if (!(p0 < p2)) return 1;
    if (!(p2 < p4)) return 2;
    if (!(p4 < pend)) return 3;

    if (!(p2 > p0)) return 4;
    if (!(p0 <= p0)) return 5;
    if (!(p4 >= p2)) return 6;
    if (p0 < p0) return 7;

    /* Consistency with subscript ordering across the whole array. */
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++) {
            int lt = (&a[i] < &a[j]);
            if (lt != (i < j)) return 8;
        }

    /* one-past-the-end compares greater than all valid element pointers */
    if (!(pend > p4)) return 9;

    return 0;
}
