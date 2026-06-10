/* LANG-6.10.8-02 — 6.10.8.1: the predefined macro __STDC__ expands to the
 * integer constant 1, and for C17 __STDC_VERSION__ expands to the integer
 * constant 201710L. Both must be usable in constant expressions at file
 * scope in a freestanding translation unit. */

_Static_assert(__STDC__ == 1, "__STDC__ must expand to 1");

#ifndef __STDC_VERSION__
#error "__STDC_VERSION__ must be predefined in C17"
#endif

_Static_assert(__STDC_VERSION__ == 201710L,
               "__STDC_VERSION__ must be 201710L for ISO C17");

/* __STDC_HOSTED__ (6.10.8.1p1) must be defined to 1 for a hosted
 * implementation or 0 for a freestanding one. */
#ifndef __STDC_HOSTED__
#error "__STDC_HOSTED__ must be predefined"
#endif

_Static_assert(__STDC_HOSTED__ == 0 || __STDC_HOSTED__ == 1,
               "__STDC_HOSTED__ must be 0 or 1");
