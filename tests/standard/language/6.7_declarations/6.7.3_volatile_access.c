/* LANG-6.7.3-05 — volatile access semantics (C17 6.7.3p8):
 * An object with volatile-qualified type may be modified in ways unknown to
 * the implementation; every access must be evaluated strictly according to
 * the rules of the abstract machine. Each load and store of a volatile
 * object must actually occur and observe the latest stored value. */
volatile int vglob = 1;

int main(void) {
    volatile int v = 5;

    /* Each read must see the most recent store. */
    v = 6;
    if (v != 6) return 1;
    v = v + 1;          /* load, add, store */
    if (v != 7) return 2;

    /* Compound assignment on a volatile lvalue: read then write. */
    v += 3;
    if (v != 10) return 3;

    /* Volatile object at file scope. */
    if (vglob != 1) return 4;
    vglob = 2;
    vglob = vglob * 4;
    if (vglob != 8) return 5;

    /* Access through a pointer to volatile. */
    volatile int *p = &v;
    *p = 42;
    if (v != 42) return 6;
    if (*p != 42) return 7;

    return 0;
}
