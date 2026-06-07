/* LANG-6.5.16-02 — Constraint: an assignment operator shall have a modifiable
 * lvalue as its left operand (ISO C17 6.5.16p2). An array name is an lvalue but
 * not a modifiable lvalue, so it cannot be the target of an assignment; a
 * conforming compiler must reject this. */

int f(void)
{
    int arr[4];
    int other[4] = {0};
    arr = other;   /* ill-formed: an array is not a modifiable lvalue */
    return arr[0];
}
