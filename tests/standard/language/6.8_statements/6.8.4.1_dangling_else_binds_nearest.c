/* LANG-6.8.4.1-02 — An `else` is associated with the lexically nearest
 * preceding `if` that is allowed by the syntax (ISO C17 6.8.4.1p3),
 * the classic "dangling else". */

int main(void)
{
    int r;

    /* The else belongs to the INNER if(0), not the outer if(1):
     * outer taken, inner not taken -> else runs -> r = 2. */
    r = 0;
    if (1)
        if (0)
            r = 1;
        else
            r = 2;
    if (r != 2) return 1;

    /* Outer not taken: nothing runs at all (else is the inner if's). */
    r = 0;
    if (0)
        if (1)
            r = 1;
        else
            r = 2;
    if (r != 0) return 2;

    /* Both taken: inner then-branch runs. */
    r = 0;
    if (1)
        if (1)
            r = 1;
        else
            r = 2;
    if (r != 1) return 3;

    /* Braces re-associate the else with the outer if. */
    r = 0;
    if (0) {
        if (1)
            r = 1;
    } else
        r = 2;
    if (r != 2) return 4;

    return 0;
}
