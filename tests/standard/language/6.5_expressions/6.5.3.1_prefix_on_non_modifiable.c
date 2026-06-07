/* LANG-6.5.3.1-02 — Constraint: the operand of prefix `++`/`--` must be a
 * modifiable lvalue of real or pointer type (ISO C17 6.5.3.1p1). Applying
 * prefix `++` to a `const`-qualified object is not a modifiable lvalue and
 * must be rejected. */

int f(void)
{
    const int n = 0;
    return ++n;   /* ill-formed: `n` is not a modifiable lvalue */
}
