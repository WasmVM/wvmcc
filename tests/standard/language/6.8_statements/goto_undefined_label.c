/* LANG-6.8.6.1-02 — `goto` must name a label in the enclosing function (6.8.6.1p1). */
int f(void) { goto nope; return 0; }
