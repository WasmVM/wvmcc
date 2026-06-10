/* LANG-6.7.6.2-04 — constraint violation (C17 6.7.6.2p2): an identifier
 * declared with a variably modified type shall be an ordinary identifier
 * with no linkage declared at block scope or function prototype scope, and
 * if it has static or thread storage duration it shall not have a variable
 * length array type. A conforming compiler must reject this TU. */
void f(int n) {
    static int a[n]; /* error: static storage duration object with VLA type */
    (void)a;
}
