/* LANG-6.5.3.3-05 — Constraint: the operand of `~` must have integer type
 * (ISO C17 6.5.3.3p1). Applying `~` to a floating-point operand must be
 * rejected by a conforming compiler. */

int f(void)
{
    double d = 1.0;
    return ~d;   /* ill-formed: `~` requires an operand of integer type */
}
