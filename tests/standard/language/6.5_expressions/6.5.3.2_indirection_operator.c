/* LANG-6.5.3.2-02 — 6.5.3.2p4: the unary `*` operator denotes indirection. If
 * the operand points to an object, the result is an lvalue designating that
 * object; if it points to a function, the result is a function designator. */

int g(void) { return 5; }

int main(void)
{
    int x = 10;
    int *p = &x;

    /* `*p` is an lvalue designating x: it reads the value. */
    if (*p != 10) return 1;

    /* `*p` is a modifiable lvalue: assigning through it updates x. */
    *p = 20;
    if (x != 20) return 2;

    /* Indirection through a function pointer yields a function designator,
     * which is callable. */
    int (*fp)(void) = g;
    if ((*fp)() != 5) return 3;

    /* Indirection on an array's decayed pointer designates the first element. */
    int a[3] = { 7, 8, 9 };
    if (*a != 7) return 4;
    *(a + 2) = 99;
    if (a[2] != 99) return 5;

    return 0;
}
