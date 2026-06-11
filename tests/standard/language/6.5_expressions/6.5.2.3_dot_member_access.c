/* LANG-6.5.2.3-01 — `.` member access yields the member value/lvalue with
 * qualifier propagation (ISO C17 6.5.2.3p3). */

struct point { int x; int y; };

int main(void)
{
    struct point p = { 3, 7 };

    /* member value */
    if (p.x != 3) return 1;
    if (p.y != 7) return 2;

    /* member designator is an lvalue: it can be assigned through */
    p.x = 11;
    if (p.x != 11) return 3;

    /* qualifier propagation: a const struct yields a const member.
     * Reading is still fine. */
    const struct point cp = { 5, 9 };
    if (cp.x != 5) return 4;
    if (cp.y != 9) return 5;

    return 0;
}
