/* LANG-6.5.6-10 — 6.5.6p2 (ISO C17): when one operand of + is a pointer, the
 * other operand must have integer type. Adding a floating-point value to a
 * pointer violates this constraint and must be rejected. Verify=compile-fail. */
int f(int *p, double d) {
    return *(p + d);   /* non-integer added to a pointer */
}
