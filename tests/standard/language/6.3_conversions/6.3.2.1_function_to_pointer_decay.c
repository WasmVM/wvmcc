/* LANG-6.3.2.1-04 — 6.3.2.1p4: function-designator decay. A function designator
 * is an expression that has function type. Except when it is the operand of the
 * sizeof operator or the unary & operator, a function designator with type
 * "function returning T" is converted to an expression with type "pointer to
 * function returning T". */

static int add(int a, int b) { return a + b; }
static int neg(int a)        { return -a; }

int main(void) {
    /* A function name in a value context decays to a function pointer. */
    int (*fp)(int, int) = add;
    if (fp != &add) return 1;             /* `add` and `&add` are the same ptr */
    if (fp != add) return 2;

    /* Calling through the decayed pointer; both call forms are equivalent. */
    if (fp(2, 3) != 5) return 3;
    if ((*fp)(2, 3) != 5) return 4;       /* *fp decays right back to a pointer */

    /* The unary & on a function yields pointer-to-function (no separate decay
     * needed); it must equal the plain designator's decayed value. */
    int (*fp2)(int, int) = &add;
    if (fp2 != fp) return 5;

    /* A different function gives a distinct pointer value. */
    int (*gp)(int) = neg;
    if (gp(7) != -7) return 6;
    if ((void *)gp == (void *)fp) return 7;

    /* Passing a function designator as an argument: it decays to a pointer. */
    int (*chosen)(int, int) = add;
    if (chosen(10, 20) != 30) return 8;

    return 0;
}
