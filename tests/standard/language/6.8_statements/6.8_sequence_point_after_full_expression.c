/* LANG-6.8-02 — There is a sequence point between the evaluation of a full
 * expression and of the next full expression to be evaluated (ISO C17 6.8p4).
 * All side effects of one statement's expression are complete before the next
 * statement's expression is evaluated. */

int main(void)
{
    int n = 0;

    /* The side effect of each expression statement (a full expression) is
     * complete before the next one reads the value. */
    n++;
    if (n != 1) return 1;

    n = n * 10 + n; /* reads the fully-updated value: 11 */
    if (n != 11) return 2;

    /* The controlling expression of an `if` is a full expression: the
     * increment is complete before the substatement executes. */
    if (n++ == 11) {
        if (n != 12) return 3;
    } else {
        return 4;
    }

    /* Initializers of automatic objects are full expressions. */
    int m = n++;        /* m = 12, then n = 13 before the next declaration */
    int k = n;
    if (m != 12) return 5;
    if (k != 13) return 6;

    return 0;
}
