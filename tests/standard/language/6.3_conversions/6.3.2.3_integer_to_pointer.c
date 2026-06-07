/* LANG-6.3.2.3-05 — 6.3.2.3p5: an integer may be converted to any pointer type.
 * Except as previously specified, the result is implementation-defined, might
 * not be correctly aligned, might not point to an entity of the referenced
 * type, and might be a trap representation. This test exercises the
 * round-trippable case the implementation must support: an address taken as an
 * integer and converted back to a usable pointer. */

int main(void) {
    int obj = 123;
    int *p = &obj;

    /* Pointer -> integer -> pointer (intptr-sized integer round-trips). */
    unsigned long as_int = (unsigned long)p;
    int *back = (int *)as_int;
    if (back != p) return 1;
    if (*back != 123) return 2;

    /* The integer 0 converts to a null pointer (the previously-specified case
     * in 6.3.2.3p3): an integer constant expression 0 is a null pointer
     * constant. A nonconstant zero integer converted to a pointer yields an
     * implementation-defined result; the constant case is well-defined. */
    int *np = (int *)0;
    if (np) return 3;

    /* Writing through the reconstructed pointer affects the original object. */
    *back = 9;
    if (obj != 9) return 4;

    return 0;
}
