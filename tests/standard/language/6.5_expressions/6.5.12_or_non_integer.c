/* LANG-6.5.12-02 — Constraint: each of the operands of the binary `|`
 * operator shall have integer type (ISO C17 6.5.12p2). Applying `|` to a
 * floating-point operand violates this constraint and must be rejected by a
 * conforming compiler. */

int f(void)
{
    double d = 1.0;
    return d | 1;   /* ill-formed: `|` requires operands of integer type */
}
