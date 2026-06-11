/* LANG-6.8.4.1-01 — `if`: the first substatement is executed if the
 * controlling expression compares unequal to 0; in the `else` form, the second
 * substatement is executed if it compares equal to 0 (ISO C17 6.8.4.1p2). */

int main(void)
{
    int taken;

    /* Nonzero: substatement executes. */
    taken = 0;
    if (1) taken = 1;
    if (taken != 1) return 1;

    /* Zero: substatement does not execute. */
    taken = 0;
    if (0) taken = 1;
    if (taken != 0) return 2;

    /* Any nonzero value counts, not just 1. */
    taken = 0;
    if (-7) taken = 1;
    if (taken != 1) return 3;

    /* else form: zero selects the second substatement. */
    taken = 0;
    if (0) taken = 1; else taken = 2;
    if (taken != 2) return 4;

    /* else form: nonzero selects the first substatement. */
    taken = 0;
    if (3) taken = 1; else taken = 2;
    if (taken != 1) return 5;

    /* Scalar (pointer) controlling expression: non-null is "true". */
    int obj = 0;
    int *p = &obj;
    taken = 0;
    if (p) taken = 1; else taken = 2;
    if (taken != 1) return 6;

    return 0;
}
