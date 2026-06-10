/* LANG-6.7.2.1-03 — within a structure, non-bit-field members have addresses
 * that increase in declaration order, and a pointer to the structure,
 * suitably converted, points to its initial member (C17 6.7.2.1p15). */
struct s {
    char a;
    int b;
    long c;
    char d;
};

int main(void) {
    struct s obj;

    /* increasing addresses in declaration order */
    if (!((char *)&obj.a < (char *)&obj.b)) return 1;
    if (!((char *)&obj.b < (char *)&obj.c)) return 2;
    if (!((char *)&obj.c < (char *)&obj.d)) return 3;

    /* a pointer to the struct, converted, points to the first member */
    if ((char *)&obj != (char *)&obj.a) return 4;

    /* and conversely the first member is at offset zero */
    obj.a = 'Q';
    if (*(char *)&obj != 'Q') return 5;

    return 0;
}
