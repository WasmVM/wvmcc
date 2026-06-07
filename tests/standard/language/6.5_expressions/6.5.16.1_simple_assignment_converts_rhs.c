/* LANG-6.5.16.1-01 — In simple assignment (`=`), the value of the right
 * operand is converted to the type of the assignment expression (the
 * unqualified type of the left operand) and replaces the value stored in the
 * object designated by the left operand (ISO C17 6.5.16.1p2). */

int main(void)
{
    /* Floating RHS converted to integer LHS (truncation toward zero). */
    {
        int i = 3.9;
        if (i != 3) return 1;
        i = -2.7;
        if (i != -2) return 2;
    }

    /* Integer RHS converted to floating LHS. */
    {
        double d;
        d = 5;
        if (d != 5.0) return 3;
    }

    /* RHS converted to the narrower LHS type (wrap/truncate to unsigned char). */
    {
        unsigned char uc;
        uc = 0x1FF;
        if (uc != 0xFF) return 4;
    }

    /* Pointer-to-int conversion via _Bool: nonzero -> 1, null -> 0. */
    {
        int x = 0;
        int *p = &x;
        _Bool b;
        b = p;
        if (b != 1) return 5;
        p = 0;
        b = p;
        if (b != 0) return 6;
    }

    /* Assignment to a pointer from a null pointer constant. */
    {
        int *q = (int *)1;
        q = 0;
        if (q != 0) return 7;
    }

    return 0;
}
