/* LANG-6.7.10-02 — _Static_assert controlling expression must be an ICE (6.7.10p3). */
int g;
_Static_assert(g, "not constant");
