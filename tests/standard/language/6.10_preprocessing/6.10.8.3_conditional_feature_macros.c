/* LANG-6.10.8-03 — 6.10.8.3: conditional-feature macros. Each of
 * __STDC_NO_ATOMICS__, __STDC_NO_COMPLEX__, __STDC_NO_THREADS__ and
 * __STDC_NO_VLA__, if defined, expands to the integer constant 1 and
 * signals that the corresponding feature is not supported. Conversely,
 * if such a macro is NOT defined, the implementation must support the
 * feature, so using it here must compile. */

/* If defined, the value must be exactly 1 (6.10.8.3p1). */
#ifdef __STDC_NO_ATOMICS__
_Static_assert(__STDC_NO_ATOMICS__ == 1, "__STDC_NO_ATOMICS__ must be 1");
#else
/* Macro absent: _Atomic types and qualifiers must be supported. */
static _Atomic int atomic_obj;
_Static_assert(sizeof(atomic_obj) > 0, "atomic object must have a size");
#endif

#ifdef __STDC_NO_COMPLEX__
_Static_assert(__STDC_NO_COMPLEX__ == 1, "__STDC_NO_COMPLEX__ must be 1");
#else
/* Macro absent: complex types must be supported. */
static _Complex double complex_obj;
_Static_assert(sizeof(complex_obj) == 2 * sizeof(double),
               "complex double is two doubles");
#endif

#ifdef __STDC_NO_THREADS__
_Static_assert(__STDC_NO_THREADS__ == 1, "__STDC_NO_THREADS__ must be 1");
#endif

#ifdef __STDC_NO_VLA__
_Static_assert(__STDC_NO_VLA__ == 1, "__STDC_NO_VLA__ must be 1");
#else
/* Macro absent: variable length arrays and variably modified types must
 * be supported (a VLA parameter declaration is a variably modified type). */
void vla_user(int n, int a[n]);
#endif
