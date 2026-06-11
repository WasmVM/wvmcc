/* LANG-6.5.7-02 — 6.5.7p5 (ISO C17): the result of `E1 >> E2` is E1
 * right-shifted E2 bit positions. If E1 has an unsigned type, or if E1 has a
 * signed type and a nonnegative value, the value of the result is the integral
 * part of the quotient of E1 / 2^E2.
 * Verify=exit: return 0 on success, a distinct non-zero code per failed check. */
int main(void) {
    /* unsigned operands */
    if ((16u >> 4) != 1u) return 1;
    if ((255u >> 1) != 127u) return 2;          /* 255 / 2 = 127 (truncated) */
    if ((1024u >> 10) != 1u) return 3;
    if ((7u >> 1) != 3u) return 4;              /* 7 / 2 = 3 */

    /* signed, nonnegative operands -> integral part of E1 / 2^E2 */
    int a = 100;
    if ((a >> 2) != 25) return 5;               /* 100 / 4 = 25 */
    if ((13 >> 1) != 6) return 6;               /* 13 / 2 = 6 */
    if ((0 >> 5) != 0) return 7;

    unsigned long long b = 1ull << 40;
    if ((b >> 40) != 1ull) return 8;

    return 0;
}
