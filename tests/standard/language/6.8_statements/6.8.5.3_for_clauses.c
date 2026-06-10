/* LANG-6.8.5.3-01 — `for (clause-1; expr-2; expr-3)`: clause-1 is evaluated
 * once before the first evaluation of the controlling expression; expr-2 is
 * evaluated before each execution of the body; expr-3 is evaluated as a void
 * expression after each execution of the body (ISO C17 6.8.5.3p1,p2).
 * An omitted expr-2 is replaced by a nonzero constant. */

int main(void)
{
    /* Init runs once; expr-2 pre-tests; expr-3 runs after each body. */
    int inits = 0;
    int tests = 0;
    int steps = 0;
    int iters = 0;
    int i;
    for (inits++, i = 0; (tests++, i < 3); steps++, i++) {
        iters++;
    }
    if (inits != 1) return 1;
    if (iters != 3) return 2;
    if (tests != 4) return 3;   /* pre-test: one more evaluation than iterations */
    if (steps != 3) return 4;   /* expr-3 once after each body execution */

    /* expr-2 pre-test: body never runs when initially false, and expr-3
     * is consequently never evaluated. */
    int ran = 0;
    int stepped = 0;
    for (i = 5; i < 5; stepped++) {
        ran = 1;
    }
    if (ran != 0) return 5;
    if (stepped != 0) return 6;

    /* Omitted expr-2 is replaced by a nonzero constant: loop until break. */
    int n = 0;
    for (i = 0;; i++) {
        n++;
        if (i == 2) break;
    }
    if (n != 3) return 7;

    return 0;
}
