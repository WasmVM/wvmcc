/* LANG-6.8.6.1-04 — A `goto` may jump from inside a nested block to a label
 * anywhere in the enclosing function (ISO C17 6.8.6.1p2/6.2.1p3: labels have
 * function scope). The nested block here needs its own dispatch loop (it
 * contains a backward goto), and the jump to the outer label must chain to
 * the enclosing block's dispatch context (#109). */

int main(void)
{
    int n = 0;

    {
    again:
        n++;
        if (n < 3) goto again;  /* backward goto inside the nested block */
        goto out;               /* leaves the nested block for an outer label */
    }

    return 7;                   /* must be skipped */
out:
    return n == 3 ? 0 : 1;
}
