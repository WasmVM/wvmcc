/* LANG-6.5.15-01 — The conditional operator `?:` evaluates the first operand,
 * and there is a sequence point between its evaluation and the evaluation of
 * the second or third operand (whichever is evaluated). The second operand is
 * evaluated only if the first compares unequal to 0; the third operand is
 * evaluated only if the first compares equal to 0; the result is the value of
 * the evaluated operand (ISO C17 6.5.15p4). */

static int side_effects;

static int bump(int ret)
{
    ++side_effects;
    return ret;
}

int main(void)
{
    /* First operand != 0: only the second operand is evaluated. */
    side_effects = 0;
    if ((1 ? bump(10) : bump(20)) != 10) return 1;
    if (side_effects != 1) return 2;

    /* First operand == 0: only the third operand is evaluated. */
    side_effects = 0;
    if ((0 ? bump(10) : bump(20)) != 20) return 3;
    if (side_effects != 1) return 4;

    /* Sequence point after the first operand: a modification in the first
     * operand is complete before the chosen operand reads the object. */
    {
        int n = 0;
        int r = ((n = 5) ? n : -1);
        if (r != 5) return 5;
        if (n != 5) return 6;
    }

    return 0;
}
