/* tests/standard/libc/wctype/types.c — LIBC-wctype-types-01 (C17 7.30.1).
 * Verify=static-assert. <wctype.h> declares the types wint_t, wctype_t and
 * wctrans_t, and defines the macro WEOF:
 *   - wint_t is an integer type unchanged by the default argument
 *     promotions that can hold any value corresponding to members of the
 *     extended character set, as well as at least one value that does not
 *     (WEOF) (7.30.1p2, 7.19p2).
 *   - wctype_t is a scalar type that can hold values representing
 *     locale-specific character classifications (7.30.1p2).
 *   - wctrans_t is a scalar type that can hold values representing
 *     locale-specific character mappings (7.30.1p2).
 *   - WEOF expands to a constant expression of type wint_t whose value does
 *     not correspond to any member of the extended character set (7.30.1p3).
 */
#include <wctype.h>

/* wint_t is a complete integer type usable in integer constant
 * expressions (7.30.1p2). */
_Static_assert(sizeof(wint_t) > 0, "wint_t is a complete type");
_Static_assert((wint_t)1 == 1, "wint_t is an integer type");

/* wint_t is unchanged by the default argument promotions (7.19p2 fn):
 * unary + applies the integer promotions; the result must still be
 * wint_t. */
_Static_assert(_Generic(+(wint_t)0, wint_t: 1, default: 0),
               "wint_t is unchanged by the integer promotions");

/* WEOF is a constant expression of type wint_t (7.30.1p3): usable in a
 * static assertion, and its type must match wint_t exactly. */
_Static_assert(_Generic(WEOF, wint_t: 1, default: 0),
               "WEOF has type wint_t");
_Static_assert(WEOF == (wint_t)WEOF, "WEOF is a constant expression");

/* wctype_t and wctrans_t are scalar types (7.30.1p2): objects of those
 * types can be defined, they have nonzero size, and a scalar value can be
 * produced by casting (casts to non-scalar types are ill-formed). */
static wctype_t  wctype_obj;
static wctrans_t wctrans_obj;
_Static_assert(sizeof(wctype_obj) > 0, "wctype_t is a complete object type");
_Static_assert(sizeof(wctrans_obj) > 0,
               "wctrans_t is a complete object type");
_Static_assert(sizeof((wctype_t)0) == sizeof(wctype_t),
               "wctype_t is a scalar type (cast from 0 is valid)");
_Static_assert(sizeof((wctrans_t)0) == sizeof(wctrans_t),
               "wctrans_t is a scalar type (cast from 0 is valid)");
