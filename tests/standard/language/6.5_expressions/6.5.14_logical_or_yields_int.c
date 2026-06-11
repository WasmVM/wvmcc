/* LANG-6.5.14-01 — The `||` operator yields 1 if either of its operands
 * compares unequal to 0, otherwise it yields 0. The result has type `int`
 * (ISO C17 6.5.14p3). */

int main(void)
{
    /* Both operands 0 -> 0. */
    if ((0 || 0) != 0) return 1;

    /* LHS != 0 -> 1. */
    if ((1 || 0) != 1) return 2;

    /* RHS != 0 (LHS == 0) -> 1. */
    if ((0 || 5) != 1) return 3;

    /* Both != 0 -> 1. */
    if ((3 || 7) != 1) return 4;

    /* Non-zero operands other than 1 still yield exactly 1. */
    if ((42 || 0) != 1) return 5;

    /* Result type is `int`: sizeof the expression equals sizeof(int). */
    if (sizeof(1 || 0) != sizeof(int)) return 6;

    return 0;
}
