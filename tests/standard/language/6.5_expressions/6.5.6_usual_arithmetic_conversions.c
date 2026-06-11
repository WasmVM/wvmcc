/* LANG-6.5.6-03 — 6.5.6p4 (ISO C17): if both operands of + or - have arithmetic
 * type, the usual arithmetic conversions (6.3.1.8) are performed. On this LP64
 * target int is 32-bit, long is 64-bit, so `int + long` has type long, and
 * `int + unsigned` has type unsigned int with modular semantics.
 * Verify=exit. */
int main(void) {
    /* int + long -> long: result type wide enough to hold the true value. */
    int i = 1;
    long big = 5000000000L; /* exceeds 32-bit range */
    long r = i + big;
    if (r != 5000000001L) return 1;

    /* int + unsigned -> unsigned int (modular). -1 converts to UINT_MAX. */
    unsigned u = 1u;
    if ((-1 + u) != 0u) return 2;        /* (UINT_MAX + 1) mod 2^32 == 0 */
    if (sizeof(-1 + u) != sizeof(unsigned)) return 3;

    /* int - long -> long. */
    if ((i - big) != -4999999999L) return 4;

    return 0;
}
