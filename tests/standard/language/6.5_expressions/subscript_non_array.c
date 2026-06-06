/* LANG-6.5.2.1-03 — subscript requires a pointer/array operand (6.5.2.1p1). */
int f(void) { int x = 0; return x[0]; }
