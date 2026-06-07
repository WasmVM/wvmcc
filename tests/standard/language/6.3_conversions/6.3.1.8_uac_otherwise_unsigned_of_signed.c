/* LANG-6.3.1.8-05 — 6.3.1.8p1: otherwise (operands of equal rank, the signed
 * type cannot represent all values of the unsigned type), both operands are
 * converted to the unsigned integer type corresponding to the signed type.
 *
 * On LP64 this is long vs. unsigned long: they share the greatest rank, and
 * signed long cannot represent every unsigned long value, so the common type is
 * unsigned long and the signed operand wraps modulo 2^64. */

int main(void) {
    /* long + unsigned long -> unsigned long. -1L becomes ULONG_MAX. */
    long sneg = -1L;
    unsigned long uz = 0ul;
    unsigned long r = sneg + uz;
    if (r != 18446744073709551615ul) return 1;   /* ULONG_MAX */

    /* Comparison is performed in unsigned long: -1L < 0ul is FALSE. */
    if (((-1L) < 0ul) != 0) return 2;

    /* A negative long minus an unsigned long still uses unsigned arithmetic. */
    long a = -2L;
    unsigned long b = 1ul;
    if ((unsigned long)(a + b) != 18446744073709551615ul) return 3;

    /* A positive signed long with a small unsigned long behaves normally. */
    long p = 5L;
    unsigned long q = 7ul;
    if (p + q != 12ul) return 4;

    return 0;
}
