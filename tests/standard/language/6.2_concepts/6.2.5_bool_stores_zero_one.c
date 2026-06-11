/* LANG-6.2.5-01 — 6.2.5p2: an object declared as _Bool is large enough to
 * store the values 0 and 1. */

/* A _Bool must be able to hold both 0 and 1 distinctly, so its width is at
 * least one byte. Conversion of any nonzero value to _Bool yields 1; 0 yields 0
 * (6.3.1.2). */
_Static_assert(sizeof(_Bool) >= 1, "_Bool occupies at least one byte");
_Static_assert((_Bool)0 == 0, "_Bool stores 0");
_Static_assert((_Bool)1 == 1, "_Bool stores 1");
_Static_assert((_Bool)5 == 1, "any nonzero value converts to 1");
