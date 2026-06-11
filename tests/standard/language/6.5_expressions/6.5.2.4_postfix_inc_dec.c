/* LANG-6.5.2.4-01 — Postfix `++`/`--` yields the OLD value of the operand and
 * stores the incremented/decremented value back into the object
 * (ISO C17 6.5.2.4p2,p3). */

int main(void)
{
    /* Integer postfix increment: result is the old value. */
    int i = 5;
    if ((i++) != 5) return 1;   /* expression value is the old value */
    if (i != 6) return 2;       /* object has been incremented */

    /* Integer postfix decrement. */
    int j = 5;
    if ((j--) != 5) return 3;
    if (j != 4) return 4;

    /* Pointer postfix increment advances by one element. */
    int a[3] = { 10, 20, 30 };
    int *p = a;
    if (*(p++) != 10) return 5;  /* old pointer dereferences to a[0] */
    if (*p != 20) return 6;      /* pointer now refers to a[1] */

    /* Floating postfix decrement. */
    double d = 2.5;
    if ((d--) != 2.5) return 7;
    if (d != 1.5) return 8;

    return 0;
}
