/* LANG-6.5.7-06 — 6.5.7p5 (ISO C17): if E1 has a signed type and a negative
 * value, the value of `E1 >> E2` is implementation-defined. wvmcc documents
 * (docs/spec.md) an arithmetic, sign-extending right shift. This test verifies
 * that documented behavior.
 * Verify=exit: return 0 on success, a distinct non-zero code per failed check. */
int main(void) {
    /* arithmetic shift: sign bit is replicated, so a negative value stays
     * negative and equals the integral part of E1 / 2^E2 rounding toward
     * negative infinity for exact powers, i.e. -8 >> 1 == -4. */
    if ((-8 >> 1) != -4) return 1;
    if ((-16 >> 2) != -4) return 2;
    if ((-1 >> 1) != -1) return 3;             /* all-ones stays all-ones */
    if ((-1 >> 31) != -1) return 4;

    long long n = -1024;
    if ((n >> 5) != -32) return 5;

    /* -7 >> 1: arithmetic shift floors toward -inf -> -4 */
    if ((-7 >> 1) != -4) return 6;

    return 0;
}
