/* LANG-6.2.5-08 — 6.2.5p10: there are three real floating types (float, double,
 * long double); the value set of float is a subset of double, which is a subset
 * of long double. A nondecreasing width chain witnesses the subset relation.
 * docs/spec.md: wvmcc aliases long double to double (still a valid superset). */

_Static_assert(sizeof(float) <= sizeof(double), "float value set <= double");
_Static_assert(sizeof(double) <= sizeof(long double), "double value set <= long double");

/* The three types are distinct spellings; values representable in the narrower
 * type remain exactly representable in the wider one. */
_Static_assert((double)1.5f == 1.5, "float value exact in double");
_Static_assert((long double)1.5 == 1.5L, "double value exact in long double");
