/* LANG-6.7.4-02 — function specifiers only on functions (C17 6.7.4p2):
 * "Function specifiers shall be used only in the declaration of an
 * identifier for a function."  Applying `inline` to an object declaration
 * is a constraint violation a conforming compiler MUST reject. */
inline int x = 0;

int main(void) { return x; }
