/* LANG-6.7.6.3-02 — constraint violation (C17 6.7.6.3p1): a function
 * declarator shall not specify a return type that is a function type or an
 * array type. A conforming compiler must reject this TU. */
int f(void)[3]; /* error: function returning array of 3 ints */
