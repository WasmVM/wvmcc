/* LANG-6.7.2-01 — 6.7.2p2: each valid multiset of type specifiers denotes the
 * same type regardless of specifier order or of optional specifiers being
 * present (`unsigned long int` == `long unsigned` == `unsigned long`, …), and
 * `char`, `signed char`, and `unsigned char` are three distinct types.
 * Type identity is checked at translation time with _Generic (the controlling
 * operands are unevaluated; the selected result is an integer constant). */

/* int / signed / signed int are the same type. */
_Static_assert(_Generic((signed)0, int: 1, default: 0),
               "`signed` denotes int");
_Static_assert(_Generic((signed int)0, int: 1, default: 0),
               "`signed int` denotes int");

/* unsigned / unsigned int. */
_Static_assert(_Generic((unsigned)0, unsigned int: 1, default: 0),
               "`unsigned` denotes unsigned int");

/* short multisets. */
_Static_assert(_Generic((short int)0, short: 1, default: 0),
               "`short int` denotes short");
_Static_assert(_Generic((signed short int)0, short: 1, default: 0),
               "`signed short int` denotes short");
_Static_assert(_Generic((unsigned short int)0, unsigned short: 1, default: 0),
               "`unsigned short int` denotes unsigned short");

/* long multisets, in permuted specifier order. */
_Static_assert(_Generic((long int)0, long: 1, default: 0),
               "`long int` denotes long");
_Static_assert(_Generic((int long)0, long: 1, default: 0),
               "`int long` denotes long (order irrelevant)");
_Static_assert(_Generic((unsigned long int)0, unsigned long: 1, default: 0),
               "`unsigned long int` denotes unsigned long");
_Static_assert(_Generic((long unsigned)0, unsigned long: 1, default: 0),
               "`long unsigned` denotes unsigned long");
_Static_assert(_Generic((int unsigned long)0, unsigned long: 1, default: 0),
               "`int unsigned long` denotes unsigned long");

/* long long multisets. */
_Static_assert(_Generic((long long int)0, long long: 1, default: 0),
               "`long long int` denotes long long");
_Static_assert(_Generic((signed long long)0, long long: 1, default: 0),
               "`signed long long` denotes long long");
_Static_assert(_Generic((int long unsigned long)0,
                        unsigned long long: 1, default: 0),
               "`int long unsigned long` denotes unsigned long long");

/* char, signed char, unsigned char are three distinct types (6.7.2p2 lists
 * `char`, `signed char`, and `unsigned char` as separate multisets; 6.2.5p15
 * makes plain char a distinct type). */
_Static_assert(_Generic((char)0,
                        char: 1, signed char: 0, unsigned char: 0, default: 0),
               "plain char is its own type");
_Static_assert(_Generic((signed char)0,
                        signed char: 1, char: 0, unsigned char: 0, default: 0),
               "`signed char` is a distinct type");
_Static_assert(_Generic((unsigned char)0,
                        unsigned char: 1, char: 0, signed char: 0, default: 0),
               "`unsigned char` is a distinct type");

/* long double, with permuted order. */
_Static_assert(_Generic((double long)0, long double: 1, default: 0),
               "`double long` denotes long double");

/* _Bool is its own type, not any flavor of int/char. */
_Static_assert(_Generic((_Bool)0, _Bool: 1, default: 0),
               "_Bool denotes _Bool");
