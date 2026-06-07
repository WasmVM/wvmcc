/* LANG-6.3.2.3-02 — 6.3.2.3p2: a pointer to a qualified or unqualified type may
 * be converted to a pointer to a more qualified version of the type; the values
 * stored in the original and converted pointers shall compare equal. Adding a
 * qualifier (T* -> const T*) preserves the pointer value. */

int main(void) {
    int x = 5;
    int *p = &x;

    /* Add const: value is preserved. */
    const int *cp = p;
    if ((const int *)p != cp) return 1;

    /* Reading through the qualified pointer sees the same object. */
    if (*cp != 5) return 2;
    x = 11;
    if (*cp != 11) return 3;

    /* Add volatile likewise preserves the value. */
    volatile int *vp = p;
    if ((volatile int *)p != vp) return 4;

    /* Add both qualifiers. */
    const volatile int *cvp = p;
    if ((const volatile int *)p != cvp) return 5;
    if (*cvp != 11) return 6;

    return 0;
}
