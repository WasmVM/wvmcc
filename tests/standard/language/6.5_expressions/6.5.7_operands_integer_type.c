/* LANG-6.5.7-03 — Constraint (ISO C17 6.5.7p2): each of the operands of a
 * bitwise shift operator shall have integer type. Using a floating operand
 * must be rejected by a conforming compiler. */

int f(double d)
{
    return d << 1;   /* ill-formed: left operand has floating type */
}
