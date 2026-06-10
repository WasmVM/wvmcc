/* LANG-6.7.8-04 — an object declared via a typedef name has exactly the
 * underlying type, including its size (C17 6.7.8p3): `typedef long X; X v;`
 * makes v a long (8 bytes under this implementation's LP64 data model). */
typedef long X;

X v;

_Static_assert(sizeof(X) == sizeof(long), "typedef name denotes the underlying type");
_Static_assert(sizeof(v) == sizeof(long), "object declared via typedef is sized as long");
_Static_assert(sizeof(v) == 8, "long is 8 bytes (LP64)");
