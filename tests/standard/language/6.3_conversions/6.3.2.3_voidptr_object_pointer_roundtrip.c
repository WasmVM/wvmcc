/* LANG-6.3.2.3-01 — 6.3.2.3p1: a pointer to void may be converted to or from a
 * pointer to any object type. A pointer to any object type may be converted to
 * a pointer to void and back again; the result shall compare equal to the
 * original pointer. */

int main(void) {
    int i = 42;
    int *pi = &i;

    /* object pointer -> void* -> object pointer round-trips to an equal value */
    void *pv = pi;
    int *back = pv;
    if (back != pi) return 1;
    if (*back != 42) return 2;

    /* Works for a different object type too. */
    double d = 3.5;
    double *pd = &d;
    void *pv2 = pd;
    double *backd = pv2;
    if (backd != pd) return 3;
    if (*backd != 3.5) return 4;

    /* Round-trip through void* preserves the ability to access the object. */
    *back = 7;
    if (i != 7) return 5;

    return 0;
}
