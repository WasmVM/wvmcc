/* LANG-6.5.7-01 — 6.5.7p4 (ISO C17): the result of `E1 << E2` is E1
 * left-shifted E2 bit positions; vacated bits are zero-filled. If E1 has an
 * unsigned type, the value of the result is E1 * 2^E2, reduced modulo
 * one more than the maximum value representable in the result type.
 * Verify=exit: return 0 on success, a distinct non-zero code per failed check. */
int main(void) {
    if ((1u << 0) != 1u) return 1;
    if ((1u << 4) != 16u) return 2;
    if ((3u << 2) != 12u) return 3;

    /* unsigned int is at least 16 bits; on a 32-bit unsigned int the high
     * bits shifted out wrap modulo 2^32. */
    unsigned int x = 0xFFFFFFFFu;
    if ((unsigned int)(x << 1) != 0xFFFFFFFEu) return 4;
    if ((unsigned int)(x << 4) != 0xFFFFFFF0u) return 5;

    /* shifting a 1 into the top bit and out wraps to 0 modulo 2^32. */
    unsigned int top = 1u << 31;
    if ((unsigned int)(top << 1) != 0u) return 6;

    /* wider unsigned type */
    unsigned long long y = 1ull;
    if ((y << 40) != (1ull << 40)) return 7;

    return 0;
}
