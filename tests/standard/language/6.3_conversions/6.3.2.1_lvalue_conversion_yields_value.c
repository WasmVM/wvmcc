/* LANG-6.3.2.1-01 — 6.3.2.1p2: lvalue conversion. Except when it is the operand
 * of sizeof, unary &, ++, --, or the left operand of . or an assignment
 * operator, an lvalue that does not have array type is converted to the value
 * stored in the designated object (and is no longer an lvalue). Reading an
 * lvalue therefore yields the stored value as a non-lvalue rvalue. */

int main(void) {
    /* Reading an lvalue yields its stored value. */
    int x = 7;
    if (x != 7) return 1;

    /* The read value is a snapshot: modifying the object afterwards does not
     * change a value already obtained from a prior read. */
    int v = x;
    x = 99;
    if (v != 7) return 2;
    if (x != 99) return 3;

    /* An lvalue read in an expression yields the value at the time of the read. */
    int a = 3, b = 4;
    int sum = a + b;          /* lvalue-to-value conversion on a and b */
    if (sum != 7) return 4;

    /* Through a pointer: *p is an lvalue; reading it gives the stored value. */
    int *p = &x;
    if (*p != 99) return 5;

    /* The converted result is an rvalue: it can initialize but is not itself an
     * assignable object. We verify the value flows correctly through chains. */
    int y = (a = 10);         /* assignment yields the assigned value as rvalue */
    if (y != 10) return 6;
    if (a != 10) return 7;

    return 0;
}
