/* LANG-6.3.2.3-09 — 6.3.2.3p8: a pointer to a function of one type may be
 * converted to a pointer to a function of another type and back again; the
 * result shall compare equal to the original pointer. */

static int twice(int x) { return x + x; }

int main(void) {
    int (*orig)(int) = twice;

    /* Convert to a differently-typed function pointer and back. */
    void (*other)(void) = (void (*)(void))orig;
    int (*back)(int) = (int (*)(int))other;

    /* The round-trip result compares equal to the original. */
    if (back != orig) return 1;

    /* Calling through the correctly-typed restored pointer works. */
    if (back(21) != 42) return 2;

    /* Round-trip via void* style cast pair through another signature. */
    long (*as_long)(long) = (long (*)(long))orig;
    int (*back2)(int) = (int (*)(int))as_long;
    if (back2 != orig) return 3;
    if (back2(5) != 10) return 4;

    return 0;
}
