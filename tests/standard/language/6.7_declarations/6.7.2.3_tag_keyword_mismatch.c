/* LANG-6.7.2.3-02 — two declarations that use the same tag shall both use the
   same choice of struct, union, or enum keyword (6.7.2.3p2, constraint).
   `T` is declared as a struct, then used with the union keyword: ill-formed. */
struct T { int a; };
union T u;
