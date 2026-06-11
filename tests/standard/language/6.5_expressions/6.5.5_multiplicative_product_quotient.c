/* LANG-6.5.5-01 — The binary `*` operator yields the product and `/` yields
 * the quotient of its operands, after the usual arithmetic conversions; for
 * integral operands `/` truncates toward zero (the algebraic quotient with any
 * fractional part discarded) (ISO C17 6.5.5p3,p4). */

int main(void)
{
    /* Product of integers. */
    if ((6 * 7) != 42) return 1;

    /* Quotient with no remainder. */
    if ((42 / 7) != 6) return 2;

    /* Integer division truncates toward zero (positive). */
    if ((7 / 2) != 3) return 3;

    /* Truncation toward zero with a negative dividend: 7/2 -> 3, so
     * -7/2 must be -3 (toward zero), not -4 (toward -inf). */
    if ((-7 / 2) != -3) return 4;

    /* Negative divisor likewise truncates toward zero. */
    if ((7 / -2) != -3) return 5;

    /* Both negative: quotient is positive, truncated toward zero. */
    if ((-7 / -2) != 3) return 6;

    /* Usual arithmetic conversions: int * double is performed in double. */
    if ((3 * 2.5) != 7.5) return 7;

    /* Floating quotient is not truncated. */
    if ((7.0 / 2.0) != 3.5) return 8;

    /* Usual arithmetic conversions promote a mixed int/double divide. */
    if ((7 / 2.0) != 3.5) return 9;

    return 0;
}
