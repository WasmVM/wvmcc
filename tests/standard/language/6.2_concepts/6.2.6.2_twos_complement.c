/* LANG-6.2.6.2-02 — 6.2.6.2p2: signed integers use one of three permitted
 * representations; docs/spec.md fixes wvmcc as two's complement. Observe the
 * defining property: the sign bit has weight -(2^(N-1)), i.e. the bit pattern
 * of -1 is all-ones and INT_MIN == ~INT_MAX. Verify=exit.
 *
 * Self-contained: no libc. Returns 0 on success, distinct non-zero on the
 * first failed check. */

int main(void)
{
    /* In two's complement, -1 has the all-ones object representation, so a
     * bitwise NOT of 0 yields -1. */
    if (~0 != -1)
        return 1;

    /* INT_MIN is the complement of INT_MAX (their value bits partition the
     * full width: ~MAX == MIN under two's complement). */
    int max = 0x7FFFFFFF;
    if (~max != (-0x7FFFFFFF - 1))
        return 2;

    /* The sign bit weighs -2^(N-1): for an 8-bit signed quantity, the pattern
     * 0x80 represents -128. Reconstruct via unsigned -> signed conversion of a
     * known two's-complement bit pattern. */
    signed char sc = (signed char)0x80;
    if (sc != -128)
        return 3;

    /* -x == ~x + 1 (two's complement negation identity). */
    int x = 12345;
    if (-x != (~x + 1))
        return 4;

    return 0;
}
