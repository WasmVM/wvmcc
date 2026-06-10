/* tests/standard/libc/locale/lconv.c — LIBC-locale-lconv-01 (C17 7.11).
 * Verify=static-assert. <locale.h> shall define struct lconv, NULL, and the
 * macros LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC, LC_TIME as
 * integer constant expressions with distinct values (C17 7.11p3). */
#include <locale.h>

/* struct lconv is a complete object type (7.11p2). */
_Static_assert(sizeof(struct lconv) >= 1, "struct lconv is a complete type");

/* NULL expands to a null pointer constant (7.11p3, 7.19p3). */
_Static_assert(NULL == 0, "NULL is a null pointer constant");

/* LC_* macros expand to integer constant expressions with distinct values
 * (7.11p3). Pairwise distinctness: */
_Static_assert(LC_ALL != LC_COLLATE, "LC_ALL != LC_COLLATE");
_Static_assert(LC_ALL != LC_CTYPE, "LC_ALL != LC_CTYPE");
_Static_assert(LC_ALL != LC_MONETARY, "LC_ALL != LC_MONETARY");
_Static_assert(LC_ALL != LC_NUMERIC, "LC_ALL != LC_NUMERIC");
_Static_assert(LC_ALL != LC_TIME, "LC_ALL != LC_TIME");
_Static_assert(LC_COLLATE != LC_CTYPE, "LC_COLLATE != LC_CTYPE");
_Static_assert(LC_COLLATE != LC_MONETARY, "LC_COLLATE != LC_MONETARY");
_Static_assert(LC_COLLATE != LC_NUMERIC, "LC_COLLATE != LC_NUMERIC");
_Static_assert(LC_COLLATE != LC_TIME, "LC_COLLATE != LC_TIME");
_Static_assert(LC_CTYPE != LC_MONETARY, "LC_CTYPE != LC_MONETARY");
_Static_assert(LC_CTYPE != LC_NUMERIC, "LC_CTYPE != LC_NUMERIC");
_Static_assert(LC_CTYPE != LC_TIME, "LC_CTYPE != LC_TIME");
_Static_assert(LC_MONETARY != LC_NUMERIC, "LC_MONETARY != LC_NUMERIC");
_Static_assert(LC_MONETARY != LC_TIME, "LC_MONETARY != LC_TIME");
_Static_assert(LC_NUMERIC != LC_TIME, "LC_NUMERIC != LC_TIME");
