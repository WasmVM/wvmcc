/* LANG-6.5.16.1-01 — In simple assignment (`=`), the value of the right
 * operand is converted to the type of the assignment expression (the
 * unqualified type of the left operand) and replaces the value stored in the
 * object designated by the left operand (ISO C17 6.5.16.1p2). */

int main(void)
{
    /* Arithmetic conversion: a double RHS is converted to int on store. */
    {
        int i = 0;
        i = 3.9;            /* converted toward zero to int 3 */
        if (i != 3) return 1;
    }

    /* Narrowing/widening between integer types. */
    {
        long l = 0;
        l = (int)-5;
        if (l != -5L) return 2;
    }

    /* Conversion from int to _Bool: nonzero -> 1, zero -> 0. */
    {
        _Bool b = 0;
        b = 42;
        if (b != 1) return 3;
        b = 0;
        if (b != 0) return 4;
    }

    /* unsigned char store truncates the value modulo 256. */
    {
        unsigned char c = 0;
        c = 0x1234;
        if (c != 0x34) return 5;
    }

    /* Null pointer constant assigned to a pointer yields a null pointer. */
    {
        int *p = (int *)1;
        p = 0;
        if (p != (void *)0) return 6;
    }

    /* void* both directions without a cast. */
    {
        int obj = 7;
        void *v = 0;
        int *q = 0;
        v = &obj;           /* int* -> void* */
        q = v;              /* void* -> int* */
        if (*q != 7) return 7;
    }

    return 0;
}
