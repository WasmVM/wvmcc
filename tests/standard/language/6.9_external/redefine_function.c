/* LANG-6.9-02 / LANG-6.9.1-01 — a function is defined at most once per program (6.9p3). */
int f(void) { return 0; }
int f(void) { return 1; }
