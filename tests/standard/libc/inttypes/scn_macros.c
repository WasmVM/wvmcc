/* tests/standard/libc/inttypes/scn_macros.c — LIBC-inttypes-SCN-01.
 * ISO C17 §7.8.1: fscanf format-specifier macros. Each SCN* macro expands to
 * a string literal suitable for concatenation into a scanf format string.
 * Catalog status: partial (scanf family deferred) — the macros themselves are
 * still required by the standard. Verify=static-assert. */
#include <inttypes.h>

/* Exact-width: d/i/o/u/x for N = 8, 16, 32, 64 (no SCNX* in the standard). */
_Static_assert(sizeof("%" SCNd8) >= 3 && sizeof("%" SCNi8) >= 3, "SCNd8/SCNi8");
_Static_assert(sizeof("%" SCNo8) >= 3 && sizeof("%" SCNu8) >= 3, "SCNo8/SCNu8");
_Static_assert(sizeof("%" SCNx8) >= 3, "SCNx8");
_Static_assert(sizeof("%" SCNd16) >= 3 && sizeof("%" SCNi16) >= 3, "SCNd16/SCNi16");
_Static_assert(sizeof("%" SCNo16) >= 3 && sizeof("%" SCNu16) >= 3, "SCNo16/SCNu16");
_Static_assert(sizeof("%" SCNx16) >= 3, "SCNx16");
_Static_assert(sizeof("%" SCNd32) >= 3 && sizeof("%" SCNi32) >= 3, "SCNd32/SCNi32");
_Static_assert(sizeof("%" SCNo32) >= 3 && sizeof("%" SCNu32) >= 3, "SCNo32/SCNu32");
_Static_assert(sizeof("%" SCNx32) >= 3, "SCNx32");
_Static_assert(sizeof("%" SCNd64) >= 3 && sizeof("%" SCNi64) >= 3, "SCNd64/SCNi64");
_Static_assert(sizeof("%" SCNo64) >= 3 && sizeof("%" SCNu64) >= 3, "SCNo64/SCNu64");
_Static_assert(sizeof("%" SCNx64) >= 3, "SCNx64");

/* LEAST / FAST variants (spot-check the 32-bit set). */
_Static_assert(sizeof("%" SCNdLEAST32) >= 3 && sizeof("%" SCNiLEAST32) >= 3, "SCN LEAST32 d/i");
_Static_assert(sizeof("%" SCNoLEAST32) >= 3 && sizeof("%" SCNuLEAST32) >= 3, "SCN LEAST32 o/u");
_Static_assert(sizeof("%" SCNxLEAST32) >= 3, "SCN LEAST32 x");
_Static_assert(sizeof("%" SCNdFAST32) >= 3 && sizeof("%" SCNiFAST32) >= 3, "SCN FAST32 d/i");
_Static_assert(sizeof("%" SCNoFAST32) >= 3 && sizeof("%" SCNuFAST32) >= 3, "SCN FAST32 o/u");
_Static_assert(sizeof("%" SCNxFAST32) >= 3, "SCN FAST32 x");

/* MAX / PTR variants. */
_Static_assert(sizeof("%" SCNdMAX) >= 3 && sizeof("%" SCNiMAX) >= 3, "SCN MAX d/i");
_Static_assert(sizeof("%" SCNoMAX) >= 3 && sizeof("%" SCNuMAX) >= 3, "SCN MAX o/u");
_Static_assert(sizeof("%" SCNxMAX) >= 3, "SCN MAX x");
_Static_assert(sizeof("%" SCNdPTR) >= 3 && sizeof("%" SCNiPTR) >= 3, "SCN PTR d/i");
_Static_assert(sizeof("%" SCNoPTR) >= 3 && sizeof("%" SCNuPTR) >= 3, "SCN PTR o/u");
_Static_assert(sizeof("%" SCNxPTR) >= 3, "SCN PTR x");

int main(void) { return 0; }
