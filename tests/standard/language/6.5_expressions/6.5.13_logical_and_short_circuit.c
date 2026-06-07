/* LANG-6.5.13-02 — `&&` guarantees left-to-right evaluation: if the first
 * operand compares equal to 0, the second operand is not evaluated. There is a
 * sequence point between the evaluation of the first and second operands
 * (ISO C17 6.5.13p4). */

static int side_effects;

static int bump(int ret)
{
    ++side_effects;
    return ret;
}

int main(void)
{
    /* LHS == 0: RHS must NOT be evaluated. */
    side_effects = 0;
    if ((0 && bump(1)) != 0) return 1;
    if (side_effects != 0) return 2;

    /* LHS != 0: RHS IS evaluated. */
    side_effects = 0;
    if ((1 && bump(1)) != 1) return 3;
    if (side_effects != 1) return 4;

    /* LHS != 0 but RHS == 0: RHS evaluated, result 0. */
    side_effects = 0;
    if ((1 && bump(0)) != 0) return 5;
    if (side_effects != 1) return 6;

    /* Sequence point between operands: the modification of `n` by the LHS is
     * complete before the RHS reads it. */
    {
        int n = 0;
        if (((n = 1) && (n == 1)) != 1) return 7;
        if (n != 1) return 8;
    }

    return 0;
}
