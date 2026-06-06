/* LANG-6.7.2.3-01 — a struct tag's content is defined at most once (6.7.2.3p1). */
struct S { int a; };
struct S { int b; };
