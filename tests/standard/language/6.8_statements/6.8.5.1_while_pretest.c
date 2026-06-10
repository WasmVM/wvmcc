/* LANG-6.8.5.1-01 — `while`: the evaluation of the controlling expression
 * takes place before each execution of the loop body (ISO C17 6.8.5.1p1).
 * A while loop whose controlling expression is initially zero must execute
 * its body zero times, and the expression must be re-evaluated before every
 * subsequent iteration. */

int main(void)
{
    /* Body must never run when the controlling expression starts false. */
    int ran = 0;
    while (0) {
        ran = 1;
    }
    if (ran != 0) return 1;

    /* The controlling expression is evaluated before each body execution:
     * count evaluations and iterations.  With i starting at 0 and limit 3,
     * the expression is evaluated 4 times and the body runs 3 times. */
    int i = 0;
    int evals = 0;
    int iters = 0;
    while ((evals++, i < 3)) {
        iters++;
        i++;
    }
    if (iters != 3) return 2;
    if (evals != 4) return 3;

    return 0;
}
