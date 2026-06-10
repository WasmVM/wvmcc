/* LANG-6.7.2.1-08 — bit-field allocation order within a storage unit is
 * implementation-defined (C17 6.7.2.1p11); wvmcc documents LSB-first
 * allocation (docs/spec.md: bit index 0 is the least significant bit, fields
 * allocated low-to-high in declaration order). This test pins the
 * documented layout. */
union pun {
    struct {
        unsigned int lo : 4;  /* documented: bits 0..3  */
        unsigned int hi : 4;  /* documented: bits 4..7  */
    } bf;
    unsigned char byte;
};

int main(void) {
    union pun u;

    u.byte = 0;
    u.bf.lo = 0x1;
    u.bf.hi = 0x2;

    /* LSB-first: lo occupies the low nibble, hi the next nibble -> 0x21 */
    if (u.byte != 0x21) return 1;

    u.byte = 0xA5;
    if (u.bf.lo != 0x5) return 2;
    if (u.bf.hi != 0xA) return 3;

    return 0;
}
