/* LANG-6.3.1.8-04 — 6.3.1.8p1: if the signed operand's type can represent all
 * values of the unsigned operand's type, the unsigned operand is converted to
 * the signed type. (LP64: 64-bit long holds every 32-bit unsigned int value.) */

int main(void) {
    /* long + unsigned int -> long. UINT_MAX is representable in long, so the
     * unsigned operand keeps its value as a (positive) long. */
    long l = -1L;
    unsigned u = 4294967295u;            /* UINT_MAX */
    long r = l + u;                      /* (-1) + 4294967295 == 4294967294 */
    if (r != 4294967294L) return 1;

    /* Comparison stays signed: -1L < UINT_MAX (both as long) is TRUE. */
    if (((-1L) < 4294967295u) != 1) return 2;

    /* A negative long plus a small unsigned int stays negative (signed math). */
    long base = -100L;
    unsigned add = 1u;
    if (base + add != -99L) return 3;

    /* The common type is long, so a result exceeding INT_MAX is not truncated. */
    long hi = 3000000000L;
    unsigned ui = 2000000000u;
    if (hi + ui != 5000000000L) return 4;

    return 0;
}
