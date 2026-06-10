/* LANG-6.7.2.1-01 — a struct-declaration-list declares a new struct/union
 * type whose members can be stored to and read back (C17 6.7.2.1p6–p8).
 * The type is complete at the closing brace. */
struct point {
    int x;
    int y;
    long tag;
};

union number {
    int i;
    long l;
};

int main(void) {
    struct point p;
    union number n;

    p.x = 3;
    p.y = -7;
    p.tag = 1000000L;

    if (p.x != 3) return 1;
    if (p.y != -7) return 2;
    if (p.tag != 1000000L) return 3;

    /* members are independent objects */
    p.x = 42;
    if (p.y != -7) return 4;

    n.i = 99;
    if (n.i != 99) return 5;
    n.l = -1L;
    if (n.l != -1L) return 6;

    return 0;
}
