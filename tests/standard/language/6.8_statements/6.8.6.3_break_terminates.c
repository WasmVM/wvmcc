/* LANG-6.8.6.3-01 — A `break` statement terminates execution of the
 * smallest enclosing `switch` or iteration statement
 * (ISO C17 6.8.6.3p2). */

int main(void)
{
    /* break terminates a while loop immediately. */
    int i = 0;
    while (1) {
        if (i == 3) break;
        i++;
    }
    if (i != 3) return 1;

    /* break terminates only the SMALLEST enclosing loop. */
    int outer = 0;
    int inner = 0;
    for (i = 0; i < 3; i++) {
        outer++;
        for (int j = 0; j < 10; j++) {
            if (j == 2) break;      /* leaves inner loop only */
            inner++;
        }
    }
    if (outer != 3) return 2;
    if (inner != 6) return 3;       /* 3 outer * 2 inner */

    /* break inside a switch that is inside a loop terminates the switch,
     * not the loop. */
    int sum = 0;
    int iters = 0;
    for (i = 0; i < 4; i++) {
        iters++;
        switch (i) {
        case 1:
            sum += 10;
            break;                  /* exits the switch only */
        default:
            sum += 1;
            break;
        }
    }
    if (iters != 4) return 4;
    if (sum != 13) return 5;        /* 1 + 10 + 1 + 1 */

    return 0;
}
