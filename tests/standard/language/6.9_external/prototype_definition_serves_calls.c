/* LANG-6.9.1-04 — a definition with a parameter type list also serves as a
 * prototype for later calls; arguments are converted as if by assignment
 * (6.9.1p7). */
int add(int a, int b) {
    return a + b;
}

int main(void) {
    char c = 3;
    long l = 4;
    /* Arguments converted to int as if by assignment via the prototype. */
    if (add(c, l) != 7) return 1;
    if (add(2.9, 1.1) != 3) return 2; /* double -> int conversion: 2 + 1 */
    return 0;
}
