/* LANG-6.7.2.1-05 — constraint violation (C17 6.7.2.1p4): the width of a
 * bit-field shall be a nonnegative integer constant expression. A negative
 * width is ill-formed and a conforming compiler must reject this TU.
 * Verify=compile-fail. */
struct s {
    int f : -1; /* error: bit-field has negative width */
};
