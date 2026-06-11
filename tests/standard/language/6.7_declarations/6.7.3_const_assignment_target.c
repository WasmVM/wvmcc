/* LANG-6.7.3-04 — assigning to a const-qualified object (C17 6.7.3p7,
 * 6.5.16p2): a modifiable lvalue is required as the left operand of an
 * assignment; an lvalue with a const-qualified type is not modifiable, so
 * a conforming compiler must reject the assignment. */
int main(void) {
    const int c = 1;
    c = 2; /* constraint violation: assignment to const-qualified object */
    return 0;
}
