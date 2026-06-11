/* LANG-6.9.1-03 — in a function definition with a parameter type list, each
 * parameter declaration shall include an identifier (except a single `void`)
 * (6.9.1p5). */
int f(int, int) {
    return 0;
}
