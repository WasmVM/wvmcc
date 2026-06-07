/* LANG-6.5.8-03 — 6.5.8p2 (constraints): one of the following shall hold for a
 * relational operator: both operands have real type; or both operands are
 * pointers to qualified or unqualified versions of compatible object types.
 * Comparing pointers to incompatible object types (int* vs double*) violates
 * the constraint and must be rejected. */

int f(void)
{
    int i = 0;
    double d = 0.0;
    int *pi = &i;
    double *pd = &d;

    return pi < pd;   /* ill-formed: pointers to incompatible object types */
}
