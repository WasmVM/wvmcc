/* LANG-5.2.2-01 — 5.2.2p2: the alphabetic escape sequences \a \b \f \n \r
 * \t \v denote alert, backspace, form feed, new line, carriage return,
 * horizontal tab and vertical tab.  Under the documented ASCII/UTF-8
 * execution charset (docs/spec.md) they map to the usual control values. */

_Static_assert('\n' == 10, "\\n (new line) == 10");
_Static_assert('\t' == 9,  "\\t (horizontal tab) == 9");
_Static_assert('\a' == 7,  "\\a (alert) == 7");
_Static_assert('\b' == 8,  "\\b (backspace) == 8");
_Static_assert('\f' == 12, "\\f (form feed) == 12");
_Static_assert('\r' == 13, "\\r (carriage return) == 13");
_Static_assert('\v' == 11, "\\v (vertical tab) == 11");
