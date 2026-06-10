/* LANG-6.6-05 — Arithmetic constant expressions (operands of arithmetic
 * type: integer constants, floating constants, enumeration constants,
 * character constants, sizeof/_Alignof) are permitted in initializers of
 * objects with static storage duration (ISO C17 6.6p7,p8). A conforming
 * compiler accepts this translation unit. */

/* Floating-typed arithmetic constant expressions in static initializers. */
static double d1 = 1.5 * 4.0 + 0.25;
static double d2 = 10.0 / 4.0;
static float f1 = (float)(3.0 - 0.5);

/* Mixed integer/character/enum operands. */
enum e { K = 4 };
static double d3 = K * 2.5;
static double d4 = 'A' + 1.0;
static long l1 = 2 * 3 + 1;
static unsigned u1 = sizeof(int) + 2u;

/* Integer-typed arithmetic constant expressions are also ICEs — verify
 * their translation-time values. */
_Static_assert(2 * 3 + 1 == 7, "integer arithmetic constant expression");
_Static_assert('A' + 1 == 66, "character constant operand");
_Static_assert(K * 2 == 8, "enumeration constant operand");
_Static_assert(sizeof(char) * 8 == 8, "sizeof operand");
