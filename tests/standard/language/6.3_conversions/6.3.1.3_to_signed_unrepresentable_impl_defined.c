/* LANG-6.3.1.3-03 — 6.3.1.3p3: when converting to a signed integer type a value
 * that cannot be represented, the result is implementation-defined (or an
 * implementation-defined signal is raised). wvmcc documents two's-complement
 * truncation with no signal (docs/spec.md); this test pins that behavior. */

int main(void) {
    /* UINT_MAX -> int : two's-complement bit pattern is -1. */
    unsigned int u = 0xFFFFFFFFu;
    int s = (int)u;
    if (s != -1) return 1;

    /* 0x80000000u -> int : INT_MIN under two's-complement truncation. */
    unsigned int u2 = 0x80000000u;
    int s2 = (int)u2;
    if (s2 != (-2147483647 - 1)) return 2;

    /* 300 -> signed char : 300 & 0xFF == 0x2C == 44 (in range, positive). */
    signed char sc = (signed char)300;
    if (sc != 44) return 3;

    /* 200 -> signed char : 0xC8 truncated, top bit set -> -56. */
    signed char sc2 = (signed char)200;
    if (sc2 != -56) return 4;

    /* long 0x1_0000_0001 -> int : low 32 bits == 1. */
    long big = 0x100000001L;
    int t = (int)big;
    if (t != 1) return 5;

    return 0;
}
