/* LANG-6.5.3.2-01 — 6.5.3.2p3: the unary `&` operator yields a pointer to its
 * operand. If the operand has type T, the result has type "pointer to T" and
 * designates the object/function the operand denotes. */

int g(void) { return 7; }

int main(void)
{
    int x = 42;
    int *p = &x;

    /* &x points at x: dereferencing recovers the value. */
    if (*p != 42) return 1;

    /* Writing through the pointer modifies the original object. */
    *p = 99;
    if (x != 99) return 2;

    /* The pointer compares equal to a re-taken address of the same object. */
    if (&x != p) return 3;

    /* Address of a function yields a usable function pointer. */
    int (*fp)(void) = &g;
    if (fp() != 7) return 4;

    /* Address of an array element designates that element. */
    int a[3] = { 1, 2, 3 };
    int *q = &a[1];
    if (*q != 2) return 5;
    *q = 20;
    if (a[1] != 20) return 6;

    return 0;
}
