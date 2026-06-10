/* LANG-6.7.9-01 — the initializer for a scalar is a single expression,
 * optionally enclosed in braces; the value is converted as in simple
 * assignment (C17 6.7.9p11). */
int main(void) {
    int a = 3.9;     /* double converted to int as in assignment (truncates) */
    int b = {42};    /* braced scalar initializer */
    double d = 5;    /* int converted to double */
    long l = {7};    /* braced, with int -> long conversion */
    void *p = 0;     /* null pointer constant converts to pointer */
    char c = 65;     /* int -> char */

    if (a != 3) return 1;
    if (b != 42) return 2;
    if (d != 5.0) return 3;
    if (l != 7L) return 4;
    if (p != (void *)0) return 5;
    if (c != 'A') return 6;
    return 0;
}
