/* LANG-6.2.3-03 — 6.2.3p1: each structure or union has a separate name space for
 * its members, so the same member name may appear in two different structs. */
struct A { int m; };
struct B { int m; };

int main(void) {
    struct A a;
    struct B b;
    a.m = 11;
    b.m = 22;
    if (a.m != 11) return 1;
    if (b.m != 22) return 2;
    return 0;
}
