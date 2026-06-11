/* LANG-6.7.2.3-04 — self-referential and forward-declared (incomplete)
   struct/union tags (6.7.2.3p7,p8). A tag is usable as an incomplete type
   until its defining declaration completes it; members may point to the
   (still-incomplete) type itself or to a forward-declared one. */

/* p8: mutually-referential structures via a forward declaration. */
struct s2;                            /* incomplete type */
struct s1 { int v; struct s2 *p; };
struct s2 { int w; struct s1 *q; };   /* now complete */

/* p7: self-referential linked node. */
struct node { int val; struct node *next; };

/* Forward-declared union tag, completed later. */
union u_fwd;
union u_fwd { int i; unsigned u; };

int main(void)
{
    struct node a, b;
    a.val = 1;
    b.val = 2;
    a.next = &b;
    b.next = 0;
    if (a.next->val != 2) return 1;

    struct s1 x;
    struct s2 y;
    x.v = 10;
    y.w = 20;
    x.p = &y;
    y.q = &x;
    if (x.p->w != 20) return 2;
    if (y.q->v != 10) return 3;

    union u_fwd u;
    u.i = 7;
    if (u.i != 7) return 4;

    return 0;
}
