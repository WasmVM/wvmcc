/* LANG-6.5.5-03 — Constraint: the operands of `%` must have integer type
 * (ISO C17 6.5.5p2). Applying `%` to a floating-point operand must be rejected
 * by a conforming compiler. (`*` and `/` accept arithmetic operands, but `%`
 * is restricted to integers.) */

int f(void)
{
    double d = 5.0;
    return d % 2;   /* ill-formed: `%` requires integer operands */
}
