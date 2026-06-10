/* LANG-6.8.5.2-01 — `do`: the evaluation of the controlling expression
 * takes place after each execution of the loop body (ISO C17 6.8.5.2p1).
 * Consequently the body executes at least once even when the controlling
 * expression is always zero. */

int main(void)
{
    /* Body runs exactly once when the controlling expression is 0. */
    int ran = 0;
    do {
        ran++;
    } while (0);
    if (ran != 1) return 1;

    /* Post-test: with i starting at 0 and limit 3, the body runs 3 times
     * and the controlling expression is evaluated 3 times (once after each
     * body execution). */
    int i = 0;
    int evals = 0;
    int iters = 0;
    do {
        iters++;
        i++;
    } while ((evals++, i < 3));
    if (iters != 3) return 2;
    if (evals != 3) return 3;

    return 0;
}
