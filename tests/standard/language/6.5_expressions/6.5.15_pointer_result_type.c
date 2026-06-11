/* LANG-6.5.15-03 — Pointer second/third operands of `?:` (ISO C17 6.5.15p6):
 *  - both pointers to compatible types -> result is a pointer to the composite,
 *    qualified with the union of both operands' qualifiers;
 *  - one operand a null pointer constant -> result has the type of the other;
 *  - one operand pointer to object type, the other pointer to void -> result is
 *    pointer to void (qualifier-merged).
 * The selected operand's value is the result. */

int main(void)
{
    int x = 7;
    int y = 9;
    int *p = &x;
    int *q = &y;

    /* Compatible pointer operands: chosen value is returned unchanged. */
    if ((1 ? p : q) != &x) return 1;
    if ((0 ? p : q) != &y) return 2;
    if (*(1 ? p : q) != 7) return 3;

    /* Result qualified with the union of operand qualifiers: assigning the
     * result of (const int* , int*) to a plain int* must be diagnosable, so we
     * verify the result is usable as const int*. */
    {
        const int *cp = (1 ? (const int *)p : q);
        if (*cp != 7) return 4;
    }

    /* Null pointer constant operand: result has the type of the other operand. */
    {
        int *r = (0 ? (int *)0 : p);
        if (r != &x) return 5;
        int *s = (1 ? (int *)0 : p);
        if (s != (int *)0) return 6;
    }

    /* void* with object pointer -> result is void*. */
    {
        void *vp = (1 ? (void *)p : q);
        if (vp != (void *)&x) return 7;
        if (*(int *)(0 ? (void *)p : (void *)q) != 9) return 8;
    }

    return 0;
}
