/* LANG-6.5.1.1-02 — _Generic constraint violation (C17 6.5.1.1p2):
 * "A generic selection shall have no more than one default generic
 * association."  Two `default` associations is a constraint violation a
 * conforming compiler MUST reject. */
int f(void) {
    return _Generic(0,
        default: 1,
        default: 2);
}
