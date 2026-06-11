/* LANG-6.5.1-03 — 6.5.1p5: a parenthesized expression is a primary expression;
 * its type and value are identical to those of the unparenthesized expression,
 * and it is an lvalue if the unparenthesized expression is an lvalue. */

int main(void) {
    int x = 3;

    /* value is preserved */
    if ((x + 1) != 4) return 1;

    /* lvalue-ness is preserved: assign through a parenthesized lvalue */
    (x) = 10;
    if (x != 10) return 2;

    /* take the address of a parenthesized lvalue */
    int *p = &(x);
    *p = 20;
    if (x != 20) return 3;

    /* type is preserved: sizeof of the parenthesized expr equals the original */
    if (sizeof(x) != sizeof((x))) return 4;

    return 0;
}
