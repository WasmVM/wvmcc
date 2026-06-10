/* LANG-6.7.5-03 — valid alignment constraint (C17 6.7.5p3, 6.2.8p4):
 * "The constant expression shall be an integral constant expression. It
 * shall evaluate to a valid fundamental alignment, or to a valid extended
 * alignment supported by the implementation ..., or to zero."  Every valid
 * alignment is a nonnegative integral power of two (6.2.8p4), so
 * _Alignas(3) is a constraint violation a conforming compiler MUST
 * reject. */

_Alignas(3) char c;

int main(void) { return c; }
