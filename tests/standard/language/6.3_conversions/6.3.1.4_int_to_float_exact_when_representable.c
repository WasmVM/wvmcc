/* LANG-6.3.1.4-03 — 6.3.1.4p2: when a value of integer type is converted to a
 * real floating type, the result is exact if the value can be represented in
 * the destination type; otherwise it is the nearest representable value (the
 * choice of which of the two nearest is implementation-defined, here
 * round-to-nearest per docs/spec.md). Verify=exit. */

int main(void) {
    /* Small integers are exactly representable in float and double. */
    if ((double)0 != 0.0) return 1;
    if ((double)42 != 42.0) return 2;
    if ((double)-42 != -42.0) return 3;
    if ((float)100 != 100.0f) return 4;

    /* All 32-bit ints fit exactly in double (53-bit mantissa). */
    long imax32 = 2147483647L;            /* INT_MAX */
    if ((double)imax32 != 2147483647.0) return 5;

    /* 2^53 is exactly representable in double; integer round-trips. */
    long p53 = 9007199254775808L;          /* 2^53 */
    if ((double)p53 != 9007199254775808.0) return 6;
    if ((long)(double)p53 != p53) return 7;

    /* float has a 24-bit mantissa: 2^24 is exact and round-trips. */
    long p24 = 16777216L;                  /* 2^24 */
    if ((float)p24 != 16777216.0f) return 8;
    if ((long)(float)p24 != p24) return 9;

    /* 2^24 + 1 is NOT representable in float; round-to-nearest gives 2^24
     * (the even of the two nearest), so the result equals (float)2^24. */
    long p24p1 = 16777217L;                /* 2^24 + 1 */
    if ((float)p24p1 != (float)p24) return 10;

    return 0;
}
