/* LANG-6.5.2.3-03 — Constraint: the first operand of `.` must have
 * structure or union type (ISO C17 6.5.2.3p1). Applying `.` to an `int`
 * must be rejected. */

int f(void)
{
    int n = 0;
    return n.x;   /* ill-formed: `n` is not a structure or union */
}
