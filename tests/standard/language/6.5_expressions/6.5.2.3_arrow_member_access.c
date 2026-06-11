/* LANG-6.5.2.3-02 — `->` member access through a pointer (ISO C17 6.5.2.3p4).
 * `E->m` designates the member `m` of the struct/union that `E` points to. */

struct point { int x; int y; };

int main(void)
{
    struct point p = { 3, 7 };
    struct point *q = &p;

    /* read through the pointer */
    if (q->x != 3) return 1;
    if (q->y != 7) return 2;

    /* designator is an lvalue: store through it */
    q->x = 11;
    if (p.x != 11) return 3;

    /* q->m is equivalent to (*q).m */
    if ((*q).y != q->y) return 4;

    return 0;
}
