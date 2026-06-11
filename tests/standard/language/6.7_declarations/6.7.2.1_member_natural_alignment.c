/* LANG-6.7.2.1-10 — each non-bit-field member of a structure/union is
 * aligned in an implementation-defined manner appropriate to its type
 * (C17 6.7.2.1p14); wvmcc documents natural alignment (docs/spec.md:
 * char 1, short 2, int 4, long 8) with padding as needed. */
#include <stddef.h>

struct ci { char c; int i; };
struct cl { char c; long l; };
struct cs { char c; short s; };

/* int members are 4-aligned: 3 bytes of padding after the char */
_Static_assert(offsetof(struct ci, i) == 4, "int member 4-aligned");
_Static_assert(sizeof(struct ci) == 8, "struct {char;int} padded to 8");

/* long members are 8-aligned */
_Static_assert(offsetof(struct cl, l) == 8, "long member 8-aligned");
_Static_assert(sizeof(struct cl) == 16, "struct {char;long} padded to 16");

/* short members are 2-aligned */
_Static_assert(offsetof(struct cs, s) == 2, "short member 2-aligned");
_Static_assert(sizeof(struct cs) == 4, "struct {char;short} padded to 4");

/* the struct's alignment follows its strictest member */
_Static_assert(_Alignof(struct ci) == _Alignof(int), "struct aligned as int");
_Static_assert(_Alignof(struct cl) == _Alignof(long), "struct aligned as long");
