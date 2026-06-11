/* LANG-6.2.7-01 — 6.2.7p1: companion TU for 6.2.7_compatible_cross_tu.c.
 *
 * Redeclares `struct point` with the SAME tag and the SAME members in the SAME
 * order. By 6.2.7p1 this struct is compatible with (the same type as) the one
 * in the main TU, so values flow across the link boundary correctly.
 */

struct point { int x; int y; };   /* identical to the main TU's struct point */

struct point aux_make(int x, int y) {
    struct point p;
    p.x = x;
    p.y = y;
    return p;
}

int aux_sum(struct point p) {
    return p.x + p.y;
}
