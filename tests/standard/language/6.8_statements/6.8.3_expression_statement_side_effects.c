/* LANG-6.8.3-01 — In an expression statement, the expression is evaluated as a
 * void expression for its side effects (ISO C17 6.8.3p2). */

static int calls;

static int bump(void)
{
    ++calls;
    return 42;
}

int main(void)
{
    int n = 0;

    n + 1;      /* evaluated; no side effect, value discarded */
    n = 5;      /* side effect: n becomes 5 */
    if (n != 5) return 1;

    n++;        /* side effect of an expression statement */
    if (n != 6) return 2;

    bump();     /* function call evaluated for side effects, value discarded */
    if (calls != 1) return 3;

    n = 0, bump(), n += 3;  /* whole comma expression evaluated, in order */
    if (n != 3) return 4;
    if (calls != 2) return 5;

    return 0;
}
