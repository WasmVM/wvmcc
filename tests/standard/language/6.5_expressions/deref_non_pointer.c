/* LANG-6.5.3.2-05 — unary * requires a pointer operand (6.5.3.2p2). */
int f(void) { int x = 0; return *x; }
