/* LANG-6.5.9-03 — 6.5.9p2: one of the following shall hold for == / != :
 *   - both operands have arithmetic type;
 *   - both operands are pointers to qualified or unqualified compatible types;
 *   - one operand is a pointer to an object type and the other is a pointer to
 *     a qualified or unqualified version of void;
 *   - one operand is a pointer and the other is a null pointer constant.
 * Comparing a pointer to one object type against a pointer to an incompatible
 * object type (here int* vs double*) satisfies none of these and is a
 * constraint violation a conforming compiler must reject. Verify=compile-fail. */

int main(void)
{
    int i = 0;
    double d = 0.0;
    int *ip = &i;
    double *dp = &d;

    return ip == dp;   /* incompatible pointer types; not void*, not null */
}
