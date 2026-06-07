/* LANG-6.5.3.3-04 — The `!` operator yields 1 if the operand compares equal
 * to 0, and 0 otherwise; `!E` is equivalent to `(0 == E)`. The result has
 * type `int` (ISO C17 6.5.3.3p5). */

int main(void)
{
    if ((!0) != 1) return 1;        /* !0 == 1 */
    if ((!1) != 0) return 2;        /* !nonzero == 0 */
    if ((!5) != 0) return 3;
    if ((!(-3)) != 0) return 4;     /* any nonzero -> 0 */

    /* Equivalence with (0 == E). */
    int e = 42;
    if ((!e) != (0 == e)) return 5;
    int z = 0;
    if ((!z) != (0 == z)) return 6;

    /* Applies to scalar (pointer) operands. */
    int x = 0;
    int *p = &x;
    if ((!p) != 0) return 7;        /* non-null pointer -> 0 */
    int *np = 0;
    if ((!np) != 1) return 8;       /* null pointer -> 1 */

    /* Result type is int: sizeof(!E) == sizeof(int). */
    if (sizeof(!0) != sizeof(int)) return 9;

    /* Floating operand. */
    double d = 0.0;
    if ((!d) != 1) return 10;

    return 0;
}
