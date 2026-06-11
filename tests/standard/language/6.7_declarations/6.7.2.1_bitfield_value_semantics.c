/* LANG-6.7.2.1-07 — bit-field value semantics and packing into storage
 * units (C17 6.7.2.1p10,p11): a bit-field is interpreted as having a signed
 * or unsigned integer type of the given width; unsigned bit-fields wrap
 * modulo 2^width; _Bool bit-fields hold 0/1; consecutive small bit-fields
 * pack into the same storage unit. */
struct bf {
    unsigned int u4 : 4;   /* values 0..15 */
    int s4 : 4;            /* values at least -7..7 (two's complement: -8..7) */
    _Bool b : 1;
    unsigned int u8 : 8;
};

struct packed {
    unsigned int a : 8;
    unsigned int b : 8;
    unsigned int c : 8;
    unsigned int d : 8;
};

int main(void) {
    struct bf x;

    /* unsigned bit-field holds its full range */
    x.u4 = 15;
    if (x.u4 != 15) return 1;

    /* unsigned bit-field arithmetic wraps modulo 2^width */
    x.u4 = 15;
    x.u4 = x.u4 + 1u; /* 16 mod 16 == 0 */
    if (x.u4 != 0) return 2;

    x.u4 = 17; /* 17 mod 16 == 1 */
    if (x.u4 != 1) return 3;

    /* signed bit-field holds small signed values */
    x.s4 = 7;
    if (x.s4 != 7) return 4;
    x.s4 = -4;
    if (x.s4 != -4) return 5;

    /* _Bool bit-field: any nonzero converts to 1 */
    x.b = 2;
    if (x.b != 1) return 6;
    x.b = 0;
    if (x.b != 0) return 7;

    /* members are independent despite sharing storage units */
    x.u4 = 9;
    x.s4 = -1;
    x.u8 = 200;
    if (x.u4 != 9) return 8;
    if (x.s4 != -1) return 9;
    if (x.u8 != 200) return 10;

    /* four 8-bit fields pack into a single 32-bit storage unit */
    if (sizeof(struct packed) != sizeof(unsigned int)) return 11;

    return 0;
}
