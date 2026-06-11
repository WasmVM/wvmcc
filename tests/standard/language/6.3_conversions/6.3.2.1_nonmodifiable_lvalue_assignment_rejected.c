/* LANG-6.3.2.1-02 — 6.3.2.1p1: a modifiable lvalue is an lvalue that does not
 * have array type, does not have an incomplete type, does not have a
 * const-qualified type, and (for structs/unions) has no const-qualified member.
 * Assignment (6.5.16.1) requires a modifiable lvalue as its left operand, a
 * constraint. Assigning to a const-qualified object is therefore ill-formed and
 * a conforming compiler MUST reject this TU.
 *
 * compile-fail: `c` is const-qualified, so it is not a modifiable lvalue and
 * cannot be the left operand of an assignment.
 */

int main(void) {
    const int c = 5;
    c = 10;          /* const object is not a modifiable lvalue — reject */
    return c;
}
