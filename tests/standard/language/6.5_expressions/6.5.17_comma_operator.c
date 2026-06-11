/* LANG-6.5.17-01 — The left operand of a comma operator is evaluated as a void
 * expression; there is a sequence point between its evaluation and that of the
 * right operand. The result has the type and value of the right operand
 * (ISO C17 6.5.17p2). */

int main(void)
{
    /* The result has the type and value of the right operand. */
    int r = (1, 2, 3);
    if (r != 3) return 1;

    /* The left operand is evaluated for its side effects, then discarded;
     * the sequence point means the store to `a` is complete before the RHS
     * (which reads `a`) is evaluated. */
    int a = 0;
    int b = (a = 5, a + 1);
    if (a != 5) return 2;
    if (b != 6) return 3;

    /* Side effects of every left operand happen, in order, left to right. */
    int n = 0;
    int v = (n += 1, n += 10, n += 100, n);
    if (n != 111) return 4;
    if (v != 111) return 5;

    /* The value/type comes solely from the right operand: a narrow left
     * operand does not influence the result type. */
    long big = (0, 0x1FFFFFFFFL);
    if (big != 0x1FFFFFFFFL) return 6;

    /* Comma operators nest/associate left to right; the overall value is the
     * rightmost operand. */
    int c = (a = 1, a = 2, a = 3);
    if (c != 3) return 7;
    if (a != 3) return 8;

    return 0;
}
