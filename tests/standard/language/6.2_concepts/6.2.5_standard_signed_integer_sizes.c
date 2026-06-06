/* LANG-6.2.5-05 — 6.2.5p4: there are five standard signed integer types
 * (signed char, short int, int, long int, long long int), each at least as wide
 * as the previous. docs/spec.md fixes wvmcc's LP64 sizes:
 * char 1, short 2, int 4, long 8, long long 8. */

_Static_assert(sizeof(signed char) == 1, "signed char is 1 byte");
_Static_assert(sizeof(short int) == 2, "short is 2 bytes");
_Static_assert(sizeof(int) == 4, "int is 4 bytes");
_Static_assert(sizeof(long int) == 8, "long is 8 bytes");
_Static_assert(sizeof(long long int) == 8, "long long is 8 bytes");

/* Non-decreasing width chain required by 6.2.5p8. */
_Static_assert(sizeof(signed char) <= sizeof(short int), "char <= short");
_Static_assert(sizeof(short int) <= sizeof(int), "short <= int");
_Static_assert(sizeof(int) <= sizeof(long int), "int <= long");
_Static_assert(sizeof(long int) <= sizeof(long long int), "long <= long long");
