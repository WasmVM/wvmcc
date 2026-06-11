/* LANG-6.2.6.1-02 — 6.2.6.1p3: unsigned char (and unsigned bit-fields) use a
 * pure binary representation: value == sum of value-bit weights 2^i, and the
 * type spans exactly [0, 2^CHAR_BIT - 1]. Verify=static-assert (freestanding).
 *
 * Compile-only; a held assertion = pass. */
#include <limits.h>

/* unsigned char holds exactly CHAR_BIT value bits in pure binary, so its max
 * is 2^CHAR_BIT - 1 with no padding/sign bits. */
_Static_assert(CHAR_BIT == 8, "this test assumes CHAR_BIT == 8");
_Static_assert((unsigned char)UCHAR_MAX == 255u, "UCHAR_MAX is 2^8 - 1");

/* Pure binary: each bit i contributes weight 2^i. Setting the top value bit of
 * an 8-bit unsigned char yields 2^7 = 128. */
_Static_assert((unsigned char)(1u << 7) == 128u, "top value bit weighs 2^7");

/* Successive shifts double the value (binary place-value), and the full set of
 * value bits sums to UCHAR_MAX. */
_Static_assert((unsigned char)((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3)
                              | (1u << 4) | (1u << 5) | (1u << 6) | (1u << 7))
                   == 255u,
               "sum of all value-bit weights == UCHAR_MAX");
