/* LANG-6.7.1-06 — taking the address of a register object is rejected
 * (C17 6.7.1p6 footnote 121, constraint in 6.5.3.2p1): "The operand of the
 * unary & operator shall be either a function designator, the result of a
 * [] or unary * operator, or an lvalue that designates an object that is not
 * a bit-field and is not declared with the register storage-class
 * specifier."  A conforming compiler MUST reject `&r` below. */
int main(void) {
    register int r = 0;
    int *p = &r; /* constraint violation */
    return *p;
}
