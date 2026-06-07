/* LANG-6.5.3.2-04 — Constraint (ISO C17 6.5.3.2p1): the operand of unary `&`
 * shall be a function designator, the result of a `[]` or unary `*` operator,
 * or an lvalue that is not a bit-field and is not declared with the `register`
 * storage-class specifier. Taking the address of a bit-field member violates
 * this constraint and must be rejected. */

struct S {
    int b : 4;
};

int main(void)
{
    struct S s = { 0 };
    int *p = &s.b;   /* ill-formed: `s.b` is a bit-field */
    return p != 0;
}
