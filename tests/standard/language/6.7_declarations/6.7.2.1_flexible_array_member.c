/* LANG-6.7.2.1-11 — the last member of a structure with more than one named
 * member may have an incomplete array type (a flexible array member); it is
 * ignored for sizeof, and with suitable storage the structure behaves as if
 * the array had the elements that fit (C17 6.7.2.1p18). */
struct fam {
    int count;
    int data[]; /* flexible array member */
};

union storage {
    struct fam s;
    char raw[sizeof(struct fam) + 4 * sizeof(int)];
};

int main(void) {
    union storage u;

    /* the FAM is ignored when computing the size of the struct */
    if (sizeof(struct fam) < sizeof(int)) return 1;
    if (sizeof(struct fam) > 2 * sizeof(int)) return 2; /* no trailing array */

    /* with storage for 4 elements, the FAM is usable as int[4] */
    u.s.count = 4;
    u.s.data[0] = 10;
    u.s.data[1] = 20;
    u.s.data[2] = 30;
    u.s.data[3] = 40;

    if (u.s.count != 4) return 3;
    if (u.s.data[0] != 10) return 4;
    if (u.s.data[1] != 20) return 5;
    if (u.s.data[2] != 30) return 6;
    if (u.s.data[3] != 40) return 7;

    /* the FAM starts at or after the end of the named members */
    if (!((char *)&u.s.data[0] >= (char *)&u.s.count + sizeof(int))) return 8;

    return 0;
}
