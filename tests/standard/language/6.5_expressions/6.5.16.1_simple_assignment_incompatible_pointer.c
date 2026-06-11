/* LANG-6.5.16.1-02 — Simple-assignment constraints (ISO C17 6.5.16.1p1): when
 * both operands are pointers, the left operand must point to a type compatible
 * with (and at least as qualified as) the type pointed to by the right operand.
 * Assigning a `const int *` to an `int *` discards the `const` qualifier and
 * violates the constraint; a conforming compiler must reject this. */

int f(void)
{
    const int ci = 0;
    const int *cp = &ci;
    int *p;
    p = cp;   /* ill-formed: assignment discards `const` qualifier */
    return *p;
}
