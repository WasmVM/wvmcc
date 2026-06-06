/* LANG-6.2.6.1-03 — 6.2.6.1p4: the number, order and encoding of the bytes of
 * an object are implementation-defined; observe the order via an unsigned char
 * view. docs/spec.md fixes wvmcc as little-endian. Verify=exit.
 *
 * Self-contained: no libc. Stores a known multi-byte value and reads its bytes
 * through an unsigned char *; checks they appear in little-endian order.
 * Returns 0 on success, distinct non-zero on the first failed check. */

int main(void)
{
    /* 32-bit pattern with distinct bytes. */
    unsigned int v = 0x04030201u;
    const unsigned char *p = (const unsigned char *)&v;

    if (sizeof(unsigned int) != 4)
        return 1; /* implementation-defined size assumed by this fixture */

    /* Little-endian: least-significant byte first. */
    if (p[0] != 0x01)
        return 2;
    if (p[1] != 0x02)
        return 3;
    if (p[2] != 0x03)
        return 4;
    if (p[3] != 0x04)
        return 5;

    return 0;
}
