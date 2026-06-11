/* tests/standard/libc/uchar/types.c — LIBC-uchar-types-01 (C17 7.28p2).
 * Verify=static-assert. <uchar.h> declares the types mbstate_t and size_t,
 * and char16_t / char32_t, which are unsigned integer types with the same
 * size, signedness, and alignment as uint_least16_t / uint_least32_t
 * respectively (and are usable in static initializers / constant
 * expressions like any other integer type). */
#include <uchar.h>
#include <stdint.h>

/* char16_t: same representation as uint_least16_t (7.28p2). */
_Static_assert(sizeof(char16_t) == sizeof(uint_least16_t),
               "char16_t has the same size as uint_least16_t");
_Static_assert(_Alignof(char16_t) == _Alignof(uint_least16_t),
               "char16_t has the same alignment as uint_least16_t");
_Static_assert((char16_t)-1 > 0, "char16_t is an unsigned integer type");

/* char32_t: same representation as uint_least32_t (7.28p2). */
_Static_assert(sizeof(char32_t) == sizeof(uint_least32_t),
               "char32_t has the same size as uint_least32_t");
_Static_assert(_Alignof(char32_t) == _Alignof(uint_least32_t),
               "char32_t has the same alignment as uint_least32_t");
_Static_assert((char32_t)-1 > 0, "char32_t is an unsigned integer type");

/* char16_t can hold at least 16 bits, char32_t at least 32 bits. */
_Static_assert((char16_t)0xFFFF == 0xFFFF, "char16_t holds 16-bit values");
_Static_assert((char32_t)0xFFFFFFFF == 0xFFFFFFFF,
               "char32_t holds 32-bit values");

/* size_t is declared by <uchar.h> (7.28p2): unsigned integer type. */
_Static_assert((size_t)-1 > 0, "size_t is unsigned");

/* mbstate_t is a complete (non-array) object type (7.28p2): an object of
 * that type can be defined and sized at file scope. */
static mbstate_t state_obj;
_Static_assert(sizeof(state_obj) > 0, "mbstate_t is a complete object type");
_Static_assert(sizeof(mbstate_t) > 0, "mbstate_t is a complete object type");
