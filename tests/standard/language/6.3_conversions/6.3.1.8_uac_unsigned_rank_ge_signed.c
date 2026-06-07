/* LANG-6.3.1.8-03 — 6.3.1.8p1: if the operand with unsigned type has rank
 * greater than or equal to the rank of the other (signed) operand, the signed
 * operand is converted to the unsigned type. */

int main(void) {
    /* int + unsigned int -> unsigned int (equal rank). A negative signed value
     * wraps modulo 2^32 when converted to unsigned. */
    int neg = -1;
    unsigned u = 0u;
    unsigned r = neg + u;        /* (unsigned)(-1) == UINT_MAX, +0 */
    if (r != 4294967295u) return 1;

    /* The comparison -1 < 0u is FALSE because -1 converts to a large unsigned. */
    if (((-1) < 0u) != 0) return 2;

    /* unsigned long + long -> unsigned long (equal rank, LP64). */
    long sneg = -1L;
    unsigned long ul = 0ul;
    unsigned long rr = sneg + ul;   /* (unsigned long)(-1) == ULONG_MAX */
    if (rr != 18446744073709551615ul) return 3;

    /* unsigned int has greater rank than (promoted) short: signed -> unsigned. */
    short ssn = -1;
    unsigned um = 0u;
    if ((unsigned)(ssn + um) != 4294967295u) return 4;

    return 0;
}
