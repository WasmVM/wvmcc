/* LANG-6.5.3.3-01 — The unary `+` operator yields the value of its
 * (integer-promoted) operand (ISO C17 6.5.3.3p2). */

int main(void)
{
    int i = 7;
    if ((+i) != 7) return 1;        /* +E yields the operand value */

    int neg = -5;
    if ((+neg) != -5) return 2;     /* preserves negative values */

    double d = 3.25;
    if ((+d) != 3.25) return 3;     /* applies to arithmetic types */

    /* Integer promotion: result type of +(char) is int, value unchanged. */
    char c = 'A';
    if ((+c) != 65) return 4;

    return 0;
}
