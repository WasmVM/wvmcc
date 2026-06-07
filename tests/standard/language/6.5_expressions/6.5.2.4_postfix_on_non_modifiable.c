/* LANG-6.5.2.4-02 — Constraint: the operand of postfix `++`/`--` must be a
 * modifiable lvalue of real or pointer type (ISO C17 6.5.2.4p1). Applying
 * postfix `++` to a `const`-qualified object is not a modifiable lvalue and
 * must be rejected. */

int f(void)
{
    const int n = 0;
    return n++;   /* ill-formed: `n` is not a modifiable lvalue */
}
