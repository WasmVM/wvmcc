/* LANG-6.5.2.2-06 — the called expression must have type pointer to function
 * (6.5.2.2p1). Calling a non-function object is a constraint violation that a
 * conforming compiler must reject. Verify=compile-fail. */
int f(void) {
    int x = 0;
    return x();   /* x is not a function or pointer-to-function */
}
