/* LANG-6.3.2.3-03 — 6.3.2.3p3: an integer constant expression with the value 0,
 * or such an expression cast to type void*, is a null pointer constant. If a
 * null pointer constant is converted to a pointer type, the resulting null
 * pointer is guaranteed to compare unequal to a pointer to any object or
 * function. */

int func(void) { return 0; }

int main(void) {
    int obj = 1;

    /* Both 0 and (void*)0 are null pointer constants. */
    int *p0 = 0;
    int *pv = (void *)0;
    if (p0 != pv) return 1;

    /* A null pointer is unequal to a pointer to any object. */
    if (p0 == &obj) return 2;

    /* A null pointer is unequal to a pointer to any function. */
    int (*fp)(void) = func;
    if ((void *)0 == (void *)fp) return 3;

    /* A null pointer tests false in a boolean context. */
    if (p0) return 4;
    if (!(&obj)) return 5;

    return 0;
}
