/* LANG-6.8.6.1-05 — A `goto` may jump *into* a nested scope when no VLA is
 * in scope there (ISO C17 6.8.6.1p1, 6.2.4p6): into compound statements at
 * any depth (forward and backward), into an if branch, and into loop bodies
 * — entering a loop body skips the init clause and the first condition test
 * (and their side effects). Lowered by entry-state propagation through the
 * dispatch loops (#109). */

static int calls = 0;
static int cond_false(void) { calls++; return 0; }

static int into_nested_compound(void)
{
    int x = 0;
    goto in;
    x = 99;                          /* skipped */
    { x = 50; { x = 51; in: x += 2; } x += 100; }
    return x == 102 ? 0 : 1;         /* in: then the two tails run */
}

static int backward_into_block(void)
{
    int n = 0;
    { L: n++; }
    if (n < 3) goto L;               /* re-enters the block */
    return n == 3 ? 0 : 2;
}

static int into_if_branch(void)
{
    int x = 0;
    goto in;
    if (cond_false()) { x = 1; in: x += 2; }
    return (x == 2 && calls == 0) ? 0 : 3;   /* condition never evaluated */
}

static int into_while_body(void)
{
    int i = 5, n = 0;
    goto in;
    while (i < 3) { n += 100; in: n++; i++; }
    /* enters at in: (n=1, i=6); then 6<3 is false and the loop exits */
    return (n == 1 && i == 6) ? 0 : 4;
}

static int into_for_body(void)
{
    int i = 40, n = 0;
    goto in;
    for (i = 0; i < 2; i++) { n += 10; in: n++; }
    /* enters at in: (n=1, i still 40); step -> 41; 41<2 false -> exit */
    return (n == 1 && i == 41) ? 0 : 5;
}

static int chained_labels(void)
{
    /* 6.8.1: a labeled statement may itself be labeled; both names target
     * the same statement. */
    int n = 0;
L1: L2: n++;
    if (n < 2) goto L2;
    if (n == 2) { n = 10; goto L1; }
    return n == 11 ? 0 : 6;
}

int main(void)
{
    int r;
    if ((r = into_nested_compound())) return r;
    if ((r = backward_into_block()))  return r;
    if ((r = into_if_branch()))       return r;
    if ((r = into_while_body()))      return r;
    if ((r = into_for_body()))        return r;
    if ((r = chained_labels()))       return r;
    return 0;
}
