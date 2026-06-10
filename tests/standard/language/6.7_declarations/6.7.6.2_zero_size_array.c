/* LANG-6.7.6.2-02 — constraint violation (C17 6.7.6.2p1): if the array size
 * is an integer constant expression, it shall have a value greater than
 * zero. A conforming compiler must reject this TU. */
int a[0]; /* error: array size must be greater than zero */
