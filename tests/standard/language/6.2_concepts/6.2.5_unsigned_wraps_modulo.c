/* LANG-6.2.5-07 — 6.2.5p9: a computation involving unsigned operands can never
 * overflow; a result that cannot be represented is reduced modulo 2^N, where N
 * is the number of value bits in the type. */
int main(void) {
    /* unsigned int: wraps modulo 2^32. */
    unsigned int umax = 0xFFFFFFFFu;
    if (umax + 1u != 0u) return 1;            /* UINT_MAX + 1 -> 0 */
    if (0u - 1u != 0xFFFFFFFFu) return 2;     /* 0 - 1 -> UINT_MAX */
    if (umax * 2u != 0xFFFFFFFEu) return 3;   /* (2^32-1)*2 mod 2^32 */

    /* unsigned char promotes to int for arithmetic; mask back to verify the
     * modulo-2^8 storage behavior of the unsigned char type itself. */
    unsigned char b = 255u;
    b = (unsigned char)(b + 1);
    if (b != 0) return 4;                      /* 255 + 1 -> 0 mod 256 */

    /* unsigned long: wraps modulo 2^64 (LP64). */
    unsigned long ulmax = 0xFFFFFFFFFFFFFFFFul;
    if (ulmax + 1ul != 0ul) return 5;
    if (0ul - 1ul != 0xFFFFFFFFFFFFFFFFul) return 6;

    return 0;
}
