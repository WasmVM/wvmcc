/* LANG-6.6-01 — Integer constant expressions are evaluated at translation
 * time and may be used wherever a constant is required: array sizes, enum
 * values, bit-field widths, and `case` labels (ISO C17 6.6p2,p6). */

/* Enumeration constant values are ICEs evaluated at translation time. */
enum e { A = 3 * 4 + 1, B = A - 6 };
_Static_assert(A == 13, "enum value from ICE");
_Static_assert(B == 7, "enum value referring to earlier enumerator");

/* Array size: an ICE fixes the array's size at translation time. */
static int arr[2 + 3];
_Static_assert(sizeof arr == 5 * sizeof(int), "array size from ICE");

/* Bit-field width must be an ICE. */
struct s {
    unsigned a : 2 + 1;
    unsigned b : (1 << 2);
};

/* sizeof and _Alignof yield ICEs usable in further constant expressions. */
_Static_assert(sizeof(char) == 1, "sizeof in an ICE");
_Static_assert((int)sizeof(int) * 8 >= 16, "ICE arithmetic over sizeof");

/* `case` labels require ICEs (verified by successful translation). */
int classify(int v)
{
    switch (v) {
    case 2 + 3:
        return 1;
    case A:
        return 2;
    case (int)sizeof(char):
        return 3;
    default:
        return 0;
    }
}
