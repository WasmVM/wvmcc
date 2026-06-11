/* tests/standard/libc/iso646/alternative_spellings.c — <iso646.h> macros.
 * Catalog: LIBC-iso646-and-01 (docs/standard/libc.md). Verify=static-assert.
 * ISO C17 7.9p1: <iso646.h> defines eleven object-like macros that expand
 * to the corresponding operator tokens:
 *   and -> &&   and_eq -> &=   bitand -> &    bitor -> |    compl -> ~
 *   not -> !    not_eq -> !=   or -> ||       or_eq -> |=   xor -> ^
 *   xor_eq -> ^=
 * Compile-only (-ffreestanding); a held assertion = pass. */
#include <iso646.h>

/* Each macro must be #defined (7.9p1). */
#ifndef and
#error "and not defined"
#endif
#ifndef and_eq
#error "and_eq not defined"
#endif
#ifndef bitand
#error "bitand not defined"
#endif
#ifndef bitor
#error "bitor not defined"
#endif
#ifndef compl
#error "compl not defined"
#endif
#ifndef not
#error "not not defined"
#endif
#ifndef not_eq
#error "not_eq not defined"
#endif
#ifndef or
#error "or not defined"
#endif
#ifndef or_eq
#error "or_eq not defined"
#endif
#ifndef xor
#error "xor not defined"
#endif
#ifndef xor_eq
#error "xor_eq not defined"
#endif

/* and -> && : logical AND (short-circuit, yields 0/1). */
_Static_assert((1 and 1) == 1, "and: 1 && 1 == 1");
_Static_assert((1 and 0) == 0, "and: 1 && 0 == 0");

/* or -> || : logical OR. */
_Static_assert((0 or 1) == 1, "or: 0 || 1 == 1");
_Static_assert((0 or 0) == 0, "or: 0 || 0 == 0");

/* not -> ! : logical NOT. */
_Static_assert((not 0) == 1, "not: !0 == 1");
_Static_assert((not 5) == 0, "not: !5 == 0");

/* not_eq -> != : inequality. */
_Static_assert((1 not_eq 2) == 1, "not_eq: 1 != 2");
_Static_assert((3 not_eq 3) == 0, "not_eq: 3 != 3 is 0");

/* bitand -> & : bitwise AND. */
_Static_assert((0xC bitand 0xA) == 0x8, "bitand: 0xC & 0xA == 0x8");

/* bitor -> | : bitwise OR. */
_Static_assert((0xC bitor 0xA) == 0xE, "bitor: 0xC | 0xA == 0xE");

/* xor -> ^ : bitwise XOR. */
_Static_assert((0xC xor 0xA) == 0x6, "xor: 0xC ^ 0xA == 0x6");

/* compl -> ~ : bitwise complement. */
_Static_assert((compl 0u bitand 0xFFu) == 0xFFu, "compl: ~0u low byte all-ones");
_Static_assert((compl 0xF0u bitand 0xFFu) == 0x0Fu, "compl: ~0xF0u low byte 0x0F");

/* Compound-assignment spellings (and_eq -> &=, or_eq -> |=, xor_eq -> ^=)
 * cannot appear in a constant expression, but token-pasting via stringization
 * checks they expand to exactly the operator tokens (7.9p1). */
#define STR_(x) #x
#define STR(x) STR_(x)
_Static_assert(STR(and_eq)[0] == '&' && STR(and_eq)[1] == '=' &&
               STR(and_eq)[2] == '\0', "and_eq expands to &=");
_Static_assert(STR(or_eq)[0] == '|' && STR(or_eq)[1] == '=' &&
               STR(or_eq)[2] == '\0', "or_eq expands to |=");
_Static_assert(STR(xor_eq)[0] == '^' && STR(xor_eq)[1] == '=' &&
               STR(xor_eq)[2] == '\0', "xor_eq expands to ^=");
