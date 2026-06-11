/* LANG-6.5.11-02 — Constraint: each of the operands of `^` must have integer
 * type (ISO C17 6.5.11p2). Applying `^` to a floating-point operand must be
 * rejected by a conforming compiler. */

int f(void)
{
    double d = 1.0;
    return d ^ 1;   /* ill-formed: `^` requires operands of integer type */
}
