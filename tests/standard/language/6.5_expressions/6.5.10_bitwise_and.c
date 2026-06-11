/* LANG-6.5.10-01 — The binary `&` operator yields the bitwise AND of its
 * operands, computed after the usual arithmetic conversions are applied to
 * both operands (ISO C17 6.5.10p3,p4). Each result bit is set iff the
 * corresponding bits of both converted operands are set. */

int main(void)
{
    /* Basic bitwise AND. */
    if ((0xF0 & 0x0F) != 0x00) return 1;
    if ((0xFF & 0x0F) != 0x0F) return 2;
    if ((0xAA & 0xCC) != 0x88) return 3;

    /* AND with all-ones leaves the value unchanged. */
    if ((0x1234 & 0xFFFF) != 0x1234) return 4;

    /* AND with zero clears all bits. */
    if ((0x1234 & 0) != 0) return 5;

    /* Usual arithmetic conversions: char operands are promoted to int,
     * the AND is performed in int, and the result has type int. */
    {
        unsigned char a = 0x3C;
        unsigned char b = 0x0F;
        if ((a & b) != 0x0C) return 6;
    }

    /* Mixed signed/unsigned: the common type from the usual arithmetic
     * conversions is unsigned int here, and the bit pattern is preserved. */
    {
        unsigned int u = 0xFFFFFFFFu;
        int s = 0x5A5A5A5A;
        if ((u & s) != 0x5A5A5A5Au) return 7;
    }

    /* Wider operands: AND on long values. */
    {
        long x = 0x00FF00FF00FF00FFL;
        long y = 0x0F0F0F0F0F0F0F0FL;
        if ((x & y) != 0x000F000F000F000FL) return 8;
    }

    return 0;
}
