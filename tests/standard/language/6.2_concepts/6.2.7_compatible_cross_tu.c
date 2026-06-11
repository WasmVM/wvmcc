/* LANG-6.2.7-01 — 6.2.7p1: two types are compatible when they are the same;
 * a structure/union/enum declared in separate translation units is compatible
 * if it has the same tag and members declared in the same order with compatible
 * types (and matching names/values). The compatible struct is therefore the
 * SAME type across TUs: a value passed across the link boundary keeps its
 * layout and members.
 *
 * Two-TU link test. The companion TU is 6.2.7_compatible_cross_tu_aux.c, which
 * declares an identical `struct point` and operates on values of that type.
 */

struct point { int x; int y; };   /* same tag + members as in the aux TU */

/* Defined in the companion TU, which redeclares an identical struct point. */
extern struct point aux_make(int x, int y);
extern int aux_sum(struct point p);

int main(void) {
    /* A struct value built in the other TU has the members we expect here. */
    struct point p = aux_make(3, 4);
    if (p.x != 3) return 1;
    if (p.y != 4) return 2;

    /* A struct value built here is read consistently by the other TU. */
    struct point q = { 10, 20 };
    if (aux_sum(q) != 30) return 3;

    return 0;
}
