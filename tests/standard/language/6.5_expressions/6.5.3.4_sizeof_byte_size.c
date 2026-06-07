/* LANG-6.5.3.4-01 — 6.5.3.4p2,p4 (ISO C17): sizeof yields the size in bytes of
 * its operand's type. sizeof(char) (and signed/unsigned char) is 1 by
 * definition. For an array, sizeof gives the total number of bytes (element
 * count * element size). For a structure, the size includes any internal and
 * trailing padding, so it is at least the sum of the member sizes and a
 * multiple of the struct's alignment.
 * Verify=static-assert (freestanding). A held assertion = pass. */

/* sizeof(char) == 1 by 6.5.3.4p4. */
_Static_assert(sizeof(char) == 1, "sizeof(char) is 1");
_Static_assert(sizeof(signed char) == 1, "sizeof(signed char) is 1");
_Static_assert(sizeof(unsigned char) == 1, "sizeof(unsigned char) is 1");

/* An array's size is the element count times the element size. */
_Static_assert(sizeof(int[10]) == 10 * sizeof(int), "array = count * elem");

/* sizeof applies to an object expression as well as a type name. */
static int arr[7];
_Static_assert(sizeof arr == 7 * sizeof(int), "array object size");
_Static_assert(sizeof arr / sizeof arr[0] == 7, "array element count");

/* A struct's size includes padding: it is a multiple of its alignment and at
 * least the sum of its members. Here char + (pad) + int. */
struct S { char c; int i; };
_Static_assert(sizeof(struct S) >= sizeof(char) + sizeof(int),
               "struct size covers its members");
_Static_assert(sizeof(struct S) % _Alignof(struct S) == 0,
               "struct size is a multiple of its alignment");
