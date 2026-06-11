/* LANG-6.5-03 — 6.5p4: some aspects of the bitwise operators (~ << >> & ^ |)
 * for signed types are implementation-defined.  wvmcc/docs/spec.md fixes a
 * two's-complement representation with arithmetic (sign-extending) right shift.
 * This test pins down that documented behavior. */

int main(void) {
    /* Two's-complement: ~x == -x - 1 for signed int. */
    int x = 5;
    if (~x != -6) return 1;
    if (~0 != -1) return 2;
    if (~(-1) != 0) return 3;

    /* Bitwise AND/OR/XOR on negative values via two's-complement bits. */
    if ((-1 & 0xF) != 0xF) return 4;          /* all-ones masked */
    if ((-2 & 1) != 0) return 5;              /* ...1110 & 1 */
    if ((-1 ^ 0) != -1) return 6;
    if ((0xF0 | 0x0F) != 0xFF) return 7;

    /* Arithmetic (sign-extending) right shift of a negative signed value. */
    if ((-8 >> 1) != -4) return 8;
    if ((-1 >> 4) != -1) return 9;            /* sign bit replicated */

    /* Left shift of a nonnegative signed value within range. */
    if ((1 << 4) != 16) return 10;
    if ((3 << 2) != 12) return 11;

    return 0;
}
