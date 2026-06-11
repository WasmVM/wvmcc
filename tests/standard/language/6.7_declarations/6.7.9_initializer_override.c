/* LANG-6.7.9-15 — if an initializer list provides two initializers for the
 * same subobject, the one appearing later in the list overrides the earlier
 * one (C17 6.7.9p19; the overridden initializer may be unevaluated). */
struct S {
    int x;
    int y;
};

int main(void) {
    int a[3] = {[0] = 1, [1] = 2, [0] = 10}; /* a[0] overridden to 10 */
    if (a[0] != 10) return 1;
    if (a[1] != 2 || a[2] != 0) return 2;

    struct S s = {.x = 1, .y = 2, .x = 5}; /* s.x overridden to 5 */
    if (s.x != 5 || s.y != 2) return 3;

    return 0;
}
