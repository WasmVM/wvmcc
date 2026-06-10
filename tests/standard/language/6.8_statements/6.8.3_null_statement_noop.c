/* LANG-6.8.3-02 — A null statement (consisting of just a semicolon) performs
 * no operations (ISO C17 6.8.3p3). */

int main(void)
{
    int n = 0;

    ;           /* null statement: no effect */
    ;;;         /* several in a row: still no effect */
    if (n != 0) return 1;

    /* A null statement can carry a label. */
    goto skip;
    return 2;
skip:
    ;
    n = 1;
    if (n != 1) return 3;

    /* A null statement as a loop body: the loop's side effects come solely
     * from its controlling/iteration expressions. */
    int i;
    for (i = 0; i < 5; ++i)
        ;
    if (i != 5) return 4;

    /* `if` with a null substatement does nothing either way. */
    if (n)
        ;
    if (n != 1) return 5;

    return 0;
}
