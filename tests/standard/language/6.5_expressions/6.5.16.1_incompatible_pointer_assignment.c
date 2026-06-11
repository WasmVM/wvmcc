/* LANG-6.5.16.1-02 — Simple-assignment constraint: when both operands are
 * pointers, the left operand's pointed-to type must (ignoring qualifiers) be
 * compatible with the right operand's, unless one side is `void*` or the right
 * side is a null pointer constant (ISO C17 6.5.16.1p1). Assigning between two
 * pointers to incompatible object types without a cast is a constraint
 * violation a conforming compiler must reject. */

int f(void)
{
    int *ip = 0;
    double d = 0.0;
    double *dp = &d;

    ip = dp;        /* ill-formed: int* and double* are not compatible */

    return *ip;
}
