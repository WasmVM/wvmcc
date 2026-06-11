/* LANG-6.9.1-02 — the declarator of a function definition shall itself specify a
 * function type; it cannot be inherited from a typedef (6.9.1p2). */
typedef int F(void);

F f { return 0; }
