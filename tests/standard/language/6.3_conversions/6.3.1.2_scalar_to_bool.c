/* LANG-6.3.1.2-01 — 6.3.1.2p1: When any scalar value is converted to _Bool,
 * the result is 0 if the value compares equal to 0; otherwise the result is 1.
 * (A NaN does not compare equal to 0, so it converts to 1.) */

int main(void)
{
    /* Integer zero -> 0. */
    if ((_Bool)0 != 0)
        return 1;

    /* Nonzero integer -> 1. */
    if ((_Bool)5 != 1)
        return 2;

    /* Negative integer -> 1. */
    if ((_Bool)(-3) != 1)
        return 3;

    /* A value whose low bits are zero but is itself nonzero -> 1.
     * (Conversion to _Bool is not a truncation to one bit.) */
    if ((_Bool)256 != 1)
        return 4;

    /* Floating-point zero -> 0. */
    if ((_Bool)0.0 != 0)
        return 5;

    /* Small nonzero float that truncates to integer 0 -> still 1,
     * because the conversion compares the value against 0, not its
     * integral part. */
    if ((_Bool)0.5 != 1)
        return 6;

    /* Negative nonzero float -> 1. */
    if ((_Bool)(-0.25) != 1)
        return 7;

    /* Pointer-as-scalar: null pointer -> 0, non-null -> 1. */
    int obj;
    int *p = 0;
    if ((_Bool)p != 0)
        return 8;
    p = &obj;
    if ((_Bool)p != 1)
        return 9;

    /* NaN compares unequal to 0, so it converts to 1. Build a NaN
     * without libc by dividing zero by zero in a way the compiler
     * cannot fold to a definite zero. */
    volatile double z = 0.0;
    double nan = z / z;
    if ((_Bool)nan != 1)
        return 10;

    return 0;
}
