/* LANG-6.5.12-01 — The binary `|` operator yields the bitwise inclusive OR of
 * its operands, computed after the usual arithmetic conversions are applied to
 * both operands (ISO C17 6.5.12p3,p4). Each result bit is set iff at least one
 * of the corresponding bits of the converted operands is set. */

int main(void)
{
    /* Basic bitwise inclusive OR. */
    if ((0xF0 | 0x0F) != 0xFF) return 1;
    if ((0xF0 | 0x00) != 0xF0) return 2;
    if ((0xAA | 0xCC) != 0xEE) return 3;

    /* OR with all-ones sets all bits. */
    if ((0x1234 | 0xFFFF) != 0xFFFF) return 4;

    /* OR with zero leaves the value unchanged. */
    if ((0x1234 | 0) != 0x1234) return 5;

    /* Usual arithmetic conversions: char operands are promoted to int,
     * the OR is performed in int, and the result has type int. */
    {
        unsigned char a = 0x30;
        unsigned char b = 0x0C;
        if ((a | b) != 0x3C) return 6;
    }

    /* Mixed signed/unsigned: the common type from the usual arithmetic
     * conversions is unsigned int here, and the bit pattern is preserved. */
    {
        unsigned int u = 0x5A5A5A5Au;
        int s = 0x0F0F0F0F;
        if ((u | s) != 0x5F5F5F5Fu) return 7;
    }

    /* Wider operands: OR on long values. */
    {
        long x = 0x00FF00FF00FF00FFL;
        long y = 0x0F0F0F0F0F0F0F0FL;
        if ((x | y) != 0x0FFF0FFF0FFF0FFFL) return 8;
    }

    return 0;
}
