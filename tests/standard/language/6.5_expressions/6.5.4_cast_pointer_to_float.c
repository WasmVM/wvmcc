/* LANG-6.5.4-02 — Constraints (ISO C17 6.5.4p2,p3,p4): unless the type name
 * specifies `void`, the type name must denote a scalar type and the operand
 * must have scalar type; conversions that involve pointers (other than as
 * permitted by 6.3.2.3) are not permitted by a cast. A pointer value may not
 * be cast to a floating type. A conforming compiler must reject this. */

int f(void)
{
    int x = 0;
    int *p = &x;
    double d = (double)p;   /* ill-formed: pointer cannot be cast to float */
    return (int)d;
}
