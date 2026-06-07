/* LANG-6.5.11-01 — The `^` operator yields the bitwise exclusive OR of its
 * operands, after the usual arithmetic conversions (ISO C17 6.5.11p4). The
 * result bit is set iff exactly one of the corresponding operand bits is set. */

int main(void)
{
    /* Basic XOR truth table at the bit level. */
    if ((0xF0 ^ 0x0F) != 0xFF) return 1;
    if ((0xFF ^ 0xFF) != 0x00) return 2;
    if ((0xAA ^ 0x55) != 0xFF) return 3;
    if ((0x00 ^ 0x5A) != 0x5A) return 4;

    /* XOR with zero is the identity. */
    if ((0x1234 ^ 0) != 0x1234) return 5;

    /* XOR is its own inverse: x ^ y ^ y == x. */
    {
        unsigned x = 0xDEADu;
        unsigned y = 0xBEEFu;
        if (((x ^ y) ^ y) != x) return 6;
    }

    /* Usual arithmetic conversions: char operands promote to int, and the
     * result has the common type. */
    {
        unsigned char a = 0xC3;
        unsigned char b = 0x3C;
        if ((a ^ b) != 0xFF) return 7;
    }

    /* Mixed-width operands: int ^ long is performed in the wider type. */
    {
        long w = 0x1000000000L ^ 0x1L;
        if (w != 0x1000000001L) return 8;
    }

    return 0;
}
