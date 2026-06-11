/* LANG-6.3.2.3-06 — 6.3.2.3p6: any pointer type may be converted to an integer
 * type. Except as previously specified, the result is implementation-defined.
 * If the result cannot be represented in the integer type, the behavior is
 * undefined. The result need not be in the range of values of any integer type.
 *
 * This test uses a wide enough integer type to represent the pointer (the
 * standard guarantees intptr_t-style round-trip support in <stdint.h>; here we
 * use unsigned long which is pointer-width on this implementation's LP64 model)
 * so the conversion is representable and well-defined to round-trip. */

int main(void) {
    int a = 1, b = 2;
    int *pa = &a;
    int *pb = &b;

    /* Distinct objects yield distinct integer representations. */
    unsigned long ia = (unsigned long)pa;
    unsigned long ib = (unsigned long)pb;
    if (ia == ib) return 1;

    /* The conversion is stable: same pointer -> same integer. */
    if ((unsigned long)pa != ia) return 2;

    /* Round-trip back to a pointer recovers the object. */
    int *back = (int *)ia;
    if (back != pa) return 3;
    if (*back != 1) return 4;

    /* A null pointer converts to integer 0 is NOT guaranteed by p6; but the
     * value compares as null when converted back. We instead verify a non-null
     * pointer converts to a nonzero integer here, which the model guarantees. */
    if (ia == 0) return 5;

    return 0;
}
