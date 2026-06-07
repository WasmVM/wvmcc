/* LANG-6.3.1.5-01 — 6.3.1.5p1: when a value of real floating type (double) is
 * converted to a narrower real floating type (float) and the value is in the
 * range of values representable, but cannot be represented exactly, the result
 * is the nearest representable value (the choice of nearest-up/nearest-down
 * being implementation-defined; docs/spec.md: round-to-nearest). When the value
 * IS exactly representable in float, the conversion is exact.
 * Verify=exit. main returns 0 on success, distinct non-zero on first failure. */

int main(void)
{
    /* 1. A value exactly representable in float survives the round trip. 0.5 is
     *    a power of two and exactly representable in both double and float. */
    {
        double d = 0.5;
        float f = (float)d;
        if ((double)f != 0.5)
            return 1;
    }

    /* 2. 1.0f / 0.25f etc. — integers and simple binary fractions are exact.
     *    16777216.0 == 2^24 is the last consecutive integer exactly
     *    representable in IEEE-754 single precision. */
    {
        double d = 16777216.0;          /* 2^24, exact in float */
        float f = (float)d;
        if ((double)f != 16777216.0)
            return 2;
    }

    /* 3. A double not representable in float rounds to the nearest float.
     *    0.1 has no exact binary representation. Converting double 0.1 to float
     *    must yield the float nearest to 0.1, which is NOT equal to double 0.1.
     *    The float-nearest value, re-widened to double, differs from 0.1. */
    {
        double d = 0.1;
        float f = (float)d;
        /* The float closest to 0.1 widened back to double is
         * 0.100000001490116119384765625, distinct from double 0.1. */
        if ((double)f == 0.1)
            return 3;                   /* must have lost precision */
        /* The rounded result must be within half a float ULP of the original;
         * for values near 0.1 the float ULP is ~7.45e-9, so the error must be
         * comfortably below 1e-7. Verifies "nearest", not an arbitrary value. */
        double diff = (double)f - 0.1;
        if (diff < 0.0)
            diff = -diff;
        if (diff > 1e-7)
            return 4;
    }

    /* 4. Round-to-nearest, not toward zero (truncation): a value whose exact
     *    float neighbors straddle it must pick the nearer one. 2^24 + 1 is not
     *    representable; its neighbors are 2^24 and 2^24 + 2. The value
     *    2^24 + 1 is exactly halfway; round-to-nearest-even picks 2^24 (even
     *    mantissa). Either way the result must be one of the two neighbors. */
    {
        double d = 16777217.0;          /* 2^24 + 1, not representable in float */
        float f = (float)d;
        double r = (double)f;
        if (r != 16777216.0 && r != 16777218.0)
            return 5;
    }

    /* 5. Direction of rounding for a clearly-nearer value: 2^24 + 3 lies between
     *    2^24 + 2 and 2^24 + 4, nearer to 2^24 + 4, so round-to-nearest must
     *    pick 2^24 + 4. */
    {
        double d = 16777219.0;          /* 2^24 + 3 */
        float f = (float)d;
        if ((double)f != 16777220.0)    /* 2^24 + 4 */
            return 6;
    }

    return 0;
}
