/* LANG-6.7.4-08 — no function specifier on main (C17 6.7.4p4):
 * "In a hosted environment, no function specifier(s) shall appear in a
 * declaration of main."  Declaring main with `inline` is a constraint
 * violation a conforming hosted compiler MUST reject. */
inline int main(void) { return 0; }
