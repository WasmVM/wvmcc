// M2-1: <limits.h>.
#ifndef _WVMCC_LIMITS_H
#define _WVMCC_LIMITS_H

#define CHAR_BIT   8

// `char` is signed in wvmcc's ABI (matches gcc/clang on most targets).
#define SCHAR_MIN  (-128)
#define SCHAR_MAX  127
#define UCHAR_MAX  255
#define CHAR_MIN   SCHAR_MIN
#define CHAR_MAX   SCHAR_MAX

#define SHRT_MIN   (-32768)
#define SHRT_MAX   32767
#define USHRT_MAX  65535

#define INT_MIN    (-2147483647 - 1)
#define INT_MAX    2147483647
#define UINT_MAX   4294967295U

// On wasm64, `long` is 64-bit (matches sizeof(void*) per wvmcc's TypeMap).
#define LONG_MIN   (-9223372036854775807LL - 1)
#define LONG_MAX   9223372036854775807LL
#define ULONG_MAX  18446744073709551615ULL

#define LLONG_MIN  LONG_MIN
#define LLONG_MAX  LONG_MAX
#define ULLONG_MAX ULONG_MAX

#define MB_LEN_MAX 4

#endif // _WVMCC_LIMITS_H
