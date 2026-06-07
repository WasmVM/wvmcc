/* LANG-6.5.13-01 — The `&&` operator yields 1 if both of its operands compare
 * unequal to 0, and 0 otherwise. The result has type `int` (ISO C17 6.5.13p3). */

int main(void)
{
    /* Both operands nonzero -> 1. */
    if ((1 && 1) != 1) return 1;
    if ((5 && -3) != 1) return 2;

    /* Either operand zero -> 0. */
    if ((0 && 1) != 0) return 3;
    if ((1 && 0) != 0) return 4;
    if ((0 && 0) != 0) return 5;

    /* Nonzero values that are not 1 still yield exactly 1. */
    if ((2 && 4) != 1) return 6;

    /* Floating-point operands compare against 0 too. */
    if ((0.5 && 2.0) != 1) return 7;
    if ((0.0 && 2.0) != 0) return 8;

    /* The result has type int: sizeof the whole expression equals sizeof(int). */
    if (sizeof(1 && 1) != sizeof(int)) return 9;

    return 0;
}
