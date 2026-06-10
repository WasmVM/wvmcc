/* LANG-6.7.10-01 — a _Static_assert declaration whose controlling constant
 * expression compares unequal to 0 has no effect; the translation unit is
 * accepted (C17 6.7.10p3). */
enum { N = 3 };

_Static_assert(1, "literal one");
_Static_assert(2 + 2 == 4, "arithmetic integer constant expression");
_Static_assert(sizeof(char) == 1, "sizeof in an ICE");
_Static_assert(N > 0, "enumeration constant in an ICE");
_Static_assert(-1 < 0 ? 1 : 0, "conditional operator in an ICE");

struct WithAssert {
    int member;
    _Static_assert(N == 3, "static assertion as a struct member declaration");
};

void block_scope(void) {
    _Static_assert('A' != 0, "static assertion at block scope");
}
