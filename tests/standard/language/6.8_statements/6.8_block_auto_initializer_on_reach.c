/* LANG-6.8-01 — A block groups declarations and statements; the initializer of
 * an object with automatic storage duration is evaluated each time the
 * declaration is reached in order of execution (ISO C17 6.8p3). */

static int evals;

static int init_value(int v)
{
    ++evals;
    return v;
}

int main(void)
{
    /* The initializer runs once per time the declaration is reached. */
    for (int i = 0; i < 3; ++i) {
        int x = init_value(i);
        if (x != i) return 1;
    }
    if (evals != 3) return 2;

    /* A declaration after statements in the same block is reached in order;
     * its initializer sees the effects of the preceding statements. */
    {
        int a = 1;
        a = a + 4;
        int b = init_value(a); /* reached after the assignment */
        if (b != 5) return 3;
    }
    if (evals != 4) return 4;

    /* A skipped declaration's initializer is not evaluated. */
    if (0) {
        int never = init_value(99);
        (void)never;
    }
    if (evals != 4) return 5;

    return 0;
}
