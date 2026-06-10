/* LANG-6.7.6.3-01 — function declarators (C17 6.7.6.3p5,p10): a prototype
 * declares the types of the parameters; the special case of an unnamed
 * parameter of type void as the only item in the list specifies that the
 * function has no parameters. */

int nullary(void);          /* prototype: no parameters */
int add(int a, int b);      /* prototype with named parameters */
int scale(int, int);        /* parameter names optional in a declaration */

int nullary(void) { return 7; }
int add(int a, int b) { return a + b; }
int scale(int v, int k) { return v * k; }

int main(void) {
    if (nullary() != 7) return 1;
    if (add(3, 4) != 7) return 2;
    if (scale(3, 5) != 15) return 3;

    /* Prototype types govern the call: arguments are converted as if by
     * assignment to the parameter types (long -> int here). */
    long big = 5L;
    if (add((int)big, 2) != 7) return 4;

    /* Pointer to function with (void) prototype. */
    int (*fp)(void) = nullary;
    if (fp() != 7) return 5;

    return 0;
}
