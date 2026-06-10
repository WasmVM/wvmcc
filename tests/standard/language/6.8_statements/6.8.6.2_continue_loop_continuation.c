/* LANG-6.8.6.2-01 — A `continue` statement causes a jump to the
 * loop-continuation portion of the smallest enclosing iteration statement,
 * i.e. to the end of the loop body (ISO C17 6.8.6.2p2). In a `for` loop,
 * expression-3 is still evaluated after `continue`. */

int main(void)
{
    /* while: continue skips the rest of the body, re-tests the condition. */
    int i = 0;
    int evens = 0;
    while (i < 10) {
        i++;
        if (i % 2 != 0) continue;
        evens++;
    }
    if (evens != 5) return 1;
    if (i != 10) return 2;

    /* for: continue jumps to the loop-continuation; expr-3 still runs,
     * so the loop cannot get stuck. */
    int steps = 0;
    int body_tail = 0;
    for (i = 0; i < 6; steps++, i++) {
        if (i < 3) continue;
        body_tail++;        /* reached only for i = 3,4,5 */
    }
    if (steps != 6) return 3;       /* expr-3 ran after every iteration */
    if (body_tail != 3) return 4;

    /* continue applies to the SMALLEST enclosing loop. */
    int inner_total = 0;
    int outer_iters = 0;
    for (i = 0; i < 3; i++) {
        outer_iters++;
        for (int j = 0; j < 4; j++) {
            if (j >= 2) continue;   /* affects inner loop only */
            inner_total++;
        }
    }
    if (outer_iters != 3) return 5;
    if (inner_total != 6) return 6;  /* 3 outer * 2 counted inner */

    /* do-while: continue jumps to the controlling-expression evaluation. */
    i = 0;
    int counted = 0;
    do {
        i++;
        if (i == 2) continue;
        counted++;
    } while (i < 4);
    if (i != 4) return 7;
    if (counted != 3) return 8;

    return 0;
}
