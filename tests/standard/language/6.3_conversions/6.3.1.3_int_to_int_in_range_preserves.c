/* LANG-6.3.1.3-01 — 6.3.1.3p1: when a value of integer type is converted to
 * another integer type and the value can be represented by the new type, it is
 * unchanged. */

int main(void) {
    /* signed int -> long, in range */
    int si = -12345;
    long sl = (long)si;
    if (sl != -12345L) return 1;

    /* unsigned int -> unsigned long, in range */
    unsigned int ui = 4000000000u;
    unsigned long ul = (unsigned long)ui;
    if (ul != 4000000000UL) return 2;

    /* long -> int, value fits in int */
    long big = 2000000000L;
    int back = (int)big;
    if (back != 2000000000) return 3;

    /* unsigned -> signed, value representable in signed */
    unsigned u = 100u;
    int s = (int)u;
    if (s != 100) return 4;

    /* signed -> unsigned, nonnegative value representable */
    int p = 7;
    unsigned q = (unsigned)p;
    if (q != 7u) return 5;

    /* char -> int preserves a small positive value */
    char c = 65; /* 'A' */
    int ci = (int)c;
    if (ci != 65) return 6;

    return 0;
}
