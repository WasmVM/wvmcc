/* LANG-6.5.6-09 — 6.5.6p2 (ISO C17): for the binary + operator, either both
 * operands have arithmetic type, or one is a pointer to a complete object type
 * and the other has integer type. Adding two pointers violates this constraint
 * and a conforming compiler must reject it. Verify=compile-fail. */
int f(int *p, int *q) {
    return *(p + q);   /* pointer + pointer is not allowed */
}
