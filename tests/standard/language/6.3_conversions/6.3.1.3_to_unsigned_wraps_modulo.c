/* LANG-6.3.1.3-02 — 6.3.1.3p2: when converting to an unsigned integer type, if
 * the value cannot be represented it is converted by repeatedly adding or
 * subtracting one more than the maximum value of the new type until the result
 * is in range (i.e. reduction modulo 2^N). */

int main(void) {
    /* -1 -> unsigned char yields UCHAR_MAX (255 on an 8-bit char). */
    unsigned char uc = (unsigned char)(-1);
    if (uc != (unsigned char)0xFF) return 1;

    /* 256 -> unsigned char wraps to 0. */
    unsigned char uc2 = (unsigned char)256;
    if (uc2 != 0) return 2;

    /* 257 -> unsigned char wraps to 1. */
    unsigned char uc3 = (unsigned char)257;
    if (uc3 != 1) return 3;

    /* -1 -> unsigned int yields UINT_MAX. */
    unsigned int ui = (unsigned int)(-1);
    if (ui != 0xFFFFFFFFu) return 4;

    /* A large signed long reduced modulo 2^16 into unsigned short. */
    long v = 70000L; /* 70000 mod 65536 == 4464 */
    unsigned short us = (unsigned short)v;
    if (us != 4464) return 5;

    /* Negative value -> unsigned: -3 mod 256 == 253 */
    unsigned char uc4 = (unsigned char)(-3);
    if (uc4 != 253) return 6;

    return 0;
}
