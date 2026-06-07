/* LANG-6.3.1.4-01 — 6.3.1.4p1: when a finite value of real floating type is
 * converted to an integer type, the fractional part is discarded (the value is
 * truncated toward zero). Verify=exit: returns 0 on success, distinct non-zero
 * on the first failed check. */

int main(void) {
    /* Positive values: fractional part dropped, no rounding. */
    if ((int)3.9 != 3) return 1;
    if ((int)3.0 != 3) return 2;
    if ((int)0.999 != 0) return 3;

    /* Negative values: truncation is toward zero, not toward -infinity. */
    if ((int)-3.9 != -3) return 4;
    if ((int)-0.999 != 0) return 5;

    /* The same rule applies to double and float source operands. */
    double d = -7.75;
    if ((int)d != -7) return 6;
    float f = 2.5f;
    if ((int)f != 2) return 7;

    /* Conversion to long likewise truncates toward zero. */
    if ((long)123.875 != 123L) return 8;
    if ((long)-123.875 != -123L) return 9;

    /* Conversion to unsigned of an in-range non-negative value truncates. */
    if ((unsigned)9.99 != 9u) return 10;

    return 0;
}
