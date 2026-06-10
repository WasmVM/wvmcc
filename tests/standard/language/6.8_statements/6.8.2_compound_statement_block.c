/* LANG-6.8.2-01 — A compound statement `{ ... }` is a block whose
 * block-item-list may freely intermix declarations and statements
 * (ISO C17 6.8.2p1, 6.8.2p2). */

int main(void)
{
    int a = 1;          /* declaration */
    a += 1;             /* statement */
    int b = a * 2;      /* declaration after a statement */
    if (b != 4) return 1;

    {                   /* nested compound statement is itself a block */
        int c = b + 1;
        c += a;
        int d = c;
        if (d != 7) return 2;
        b = d;
    }
    if (b != 7) return 3;

    /* An inner block's declarations do not affect the outer block. */
    int c = 100;        /* distinct from the inner `c` */
    if (c != 100) return 4;

    return 0;
}
