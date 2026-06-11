/* LANG-6.7.2.1-04 — the size of a union is sufficient to contain the largest
 * of its members, and a pointer to a union, suitably converted, points to
 * each of its members (C17 6.7.2.1p16). */
union u {
    char c;
    int i;
    long l;
};

int main(void) {
    union u obj;

    /* large enough for the largest member */
    if (sizeof(union u) < sizeof(long)) return 1;
    if (sizeof(union u) < sizeof(int)) return 2;

    /* a pointer to the union points to each member */
    if ((char *)&obj != (char *)&obj.c) return 3;
    if ((char *)&obj != (char *)&obj.i) return 4;
    if ((char *)&obj != (char *)&obj.l) return 5;

    /* members overlap: the value stored last is the one present */
    obj.l = -1L;
    obj.c = 'A';
    if (obj.c != 'A') return 6;

    return 0;
}
