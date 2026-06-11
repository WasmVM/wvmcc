/* LANG-6.5.4-01 — `(type)expr` converts the value of the operand to the
 * named (unqualified) type (ISO C17 6.5.4p5). The conversions are those of
 * 6.3, applied as if by assignment to an object of that type. */

int main(void)
{
    /* Floating to integer: truncation toward zero. */
    if ((int)3.9 != 3) return 1;
    if ((int)-3.9 != -3) return 2;

    /* Integer to floating. */
    if ((double)7 != 7.0) return 3;

    /* Narrowing integer conversion: value wraps modulo 2^N for unsigned. */
    if ((unsigned char)300 != (300 % 256)) return 4;   /* 300 -> 44 */

    /* Signed-to-unsigned conversion is modular. */
    if ((unsigned)-1 != 4294967295u) return 5;

    /* The named type is unqualified: casting to a const-qualified type
     * yields the same value as the unqualified type. */
    if ((const int)5 != 5) return 6;

    /* Cast preserves arithmetic value when target can represent it. */
    long l = 1234567;
    if ((int)l != 1234567) return 7;

    /* Pointer-to-void round trip preserves the pointer value. */
    int x = 42;
    void *p = (void *)&x;
    if (*(int *)p != 42) return 8;

    return 0;
}
