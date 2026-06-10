/* LANG-6.7.2.2-03 — ISO C17 §6.7.2.2p4
 * Each enumerated type is compatible with an implementation-defined integer
 * type capable of representing all its members. wvmcc documents (docs/spec.md)
 * that this type is int when all enumerator values are representable in int.
 * Freestanding: file-scope _Static_assert only.
 */

enum small { SMALL_MIN = -1, SMALL_MAX = 127 };

_Static_assert(sizeof(enum small) == sizeof(int),
               "enum compatible type is int when values fit in int");
_Static_assert(_Alignof(enum small) == _Alignof(int),
               "enum alignment matches its compatible type int");
_Static_assert((enum small)SMALL_MIN < 0,
               "compatible type is signed (int) for negative enumerators");
