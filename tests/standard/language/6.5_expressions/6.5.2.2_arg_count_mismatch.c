/* LANG-6.5.2.2-07 — for a call to a function with a prototype, the number of
 * arguments must agree with the number of parameters (6.5.2.2p2). Passing too
 * many arguments is a constraint violation a conforming compiler must reject.
 * Verify=compile-fail. */
static int g(int a, int b) { return a + b; }
int f(void) {
    return g(1, 2, 3);   /* too many arguments for prototype of g */
}
