/* LANG-6.5.15-04 — Constraints on the conditional operator (ISO C17
 * 6.5.15p2,p3). The first operand shall have scalar type, and the second and
 * third operands shall satisfy one of the allowed combinations (both
 * arithmetic; same struct/union; both void; compatible pointers; a pointer and
 * a null pointer constant; a pointer to object and a pointer to void). A
 * pointer and an arithmetic operand together is none of these and must be
 * rejected by a conforming compiler. */

int f(int cond)
{
    int x = 0;
    int *p = &x;
    /* ill-formed: second operand is a pointer, third operand is arithmetic. */
    return *(cond ? p : 1);
}
