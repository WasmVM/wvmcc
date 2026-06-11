/* LANG-6.5.3.3-03 — The `~` operator yields the bitwise complement of its
 * (integer-promoted) operand. For an unsigned operand, `~E == UMAX - E`
 * where UMAX is the largest value of the promoted type (ISO C17 6.5.3.3p4). */

int main(void)
{
    /* ~E == -E - 1 for two's-complement signed ints. */
    if ((~0) != -1) return 1;
    if ((~5) != -6) return 2;
    if ((~(-1)) != 0) return 3;

    /* Unsigned: ~E == UMAX - E. For 32-bit unsigned, UMAX == 4294967295u. */
    unsigned u = 0u;
    if ((~u) != 4294967295u) return 4;        /* ~0u == UMAX */

    unsigned v = 5u;
    if ((~v) != (4294967295u - 5u)) return 5; /* ~E == UMAX - E */

    /* Integer promotion applies before the operation. */
    unsigned char cu = 0;
    if ((~cu) != -1) return 6;  /* promoted to int, ~0 == -1 */

    return 0;
}
