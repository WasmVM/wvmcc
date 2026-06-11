/* LANG-6.5.3.1-01 — Prefix `++E`/`--E` is equivalent to `E+=1`/`E-=1`; the
 * result is the NEW value of the operand after the increment/decrement
 * (ISO C17 6.5.3.1p2,p3). */

int main(void)
{
    /* Integer prefix increment: result is the new value. */
    int i = 5;
    if ((++i) != 6) return 1;   /* expression value is the new value */
    if (i != 6) return 2;       /* object has been incremented */

    /* Integer prefix decrement. */
    int j = 5;
    if ((--j) != 4) return 3;
    if (j != 4) return 4;

    /* Pointer prefix increment advances by one element and yields the new
     * pointer. */
    int a[3] = { 10, 20, 30 };
    int *p = a;
    if (*(++p) != 20) return 5;  /* new pointer dereferences to a[1] */
    if (*p != 20) return 6;

    /* Floating prefix decrement. */
    double d = 2.5;
    if ((--d) != 1.5) return 7;
    if (d != 1.5) return 8;

    /* `++E` is equivalent to `E += 1`. */
    int k = 0;
    if ((++k) != (0 + 1)) return 9;

    return 0;
}
