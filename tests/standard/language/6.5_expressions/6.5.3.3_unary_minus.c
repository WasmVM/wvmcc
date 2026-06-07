/* LANG-6.5.3.3-02 — The unary `-` operator yields the negative of its
 * (integer-promoted) operand (ISO C17 6.5.3.3p3). */

int main(void)
{
    int i = 7;
    if ((-i) != -7) return 1;       /* negate a positive value */

    int neg = -5;
    if ((-neg) != 5) return 2;      /* negate a negative value */

    if ((-0) != 0) return 3;        /* negation of zero is zero */

    double d = 3.25;
    if ((-d) != -3.25) return 4;    /* applies to arithmetic types */

    /* Unsigned: -E is computed modulo 2^N. For 32-bit unsigned,
     * -1u == UINT_MAX == 4294967295u. */
    unsigned u = 1u;
    if ((-u) != 4294967295u) return 5;

    return 0;
}
