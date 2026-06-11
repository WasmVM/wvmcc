/* LANG-6.7.1-02 — at most one storage-class specifier (C17 6.7.1p2):
 * "At most, one storage-class specifier may be given in the declaration
 * specifiers in a declaration, except that _Thread_local may appear with
 * static or extern."  `static extern` is a constraint violation a conforming
 * compiler MUST reject. */
static extern int x;

int main(void) { return x; }
