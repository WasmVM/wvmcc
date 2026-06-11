/* LANG-6.5.1-01 — 6.5.1p2: an identifier designating an object is a primary
 * expression and is an lvalue (it may be read and assigned through). */

int main(void) {
    int x = 7;

    /* read through the lvalue */
    if (x != 7) return 1;

    /* assign through the lvalue */
    x = 42;
    if (x != 42) return 2;

    /* the identifier as an operand of & yields its address (lvalue-ness) */
    int *p = &x;
    *p = 99;
    if (x != 99) return 3;

    return 0;
}
