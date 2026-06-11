/* tests/standard/libc/inttypes/pri_macros.c — LIBC-inttypes-PRI-01.
 * ISO C17 §7.8.1: fprintf format-specifier macros. Each PRI* macro expands to
 * a string literal suitable for concatenation into a format string, so
 * "%" PRIxN must concatenate at translation time (a non-string expansion is a
 * constraint violation here). Verify=static-assert. */
#include <inttypes.h>

/* Exact-width: d/i/o/u/x/X for N = 8, 16, 32, 64.
 * sizeof includes the NUL, so "%" + at least one specifier char gives >= 3. */
_Static_assert(sizeof("%" PRId8) >= 3 && sizeof("%" PRIi8) >= 3, "PRId8/PRIi8");
_Static_assert(sizeof("%" PRIo8) >= 3 && sizeof("%" PRIu8) >= 3, "PRIo8/PRIu8");
_Static_assert(sizeof("%" PRIx8) >= 3 && sizeof("%" PRIX8) >= 3, "PRIx8/PRIX8");
_Static_assert(sizeof("%" PRId16) >= 3 && sizeof("%" PRIi16) >= 3, "PRId16/PRIi16");
_Static_assert(sizeof("%" PRIo16) >= 3 && sizeof("%" PRIu16) >= 3, "PRIo16/PRIu16");
_Static_assert(sizeof("%" PRIx16) >= 3 && sizeof("%" PRIX16) >= 3, "PRIx16/PRIX16");
_Static_assert(sizeof("%" PRId32) >= 3 && sizeof("%" PRIi32) >= 3, "PRId32/PRIi32");
_Static_assert(sizeof("%" PRIo32) >= 3 && sizeof("%" PRIu32) >= 3, "PRIo32/PRIu32");
_Static_assert(sizeof("%" PRIx32) >= 3 && sizeof("%" PRIX32) >= 3, "PRIx32/PRIX32");
_Static_assert(sizeof("%" PRId64) >= 3 && sizeof("%" PRIi64) >= 3, "PRId64/PRIi64");
_Static_assert(sizeof("%" PRIo64) >= 3 && sizeof("%" PRIu64) >= 3, "PRIo64/PRIu64");
_Static_assert(sizeof("%" PRIx64) >= 3 && sizeof("%" PRIX64) >= 3, "PRIx64/PRIX64");

/* LEAST / FAST variants (spot-check the 32-bit set across all six letters). */
_Static_assert(sizeof("%" PRIdLEAST32) >= 3 && sizeof("%" PRIiLEAST32) >= 3, "PRI LEAST32 d/i");
_Static_assert(sizeof("%" PRIoLEAST32) >= 3 && sizeof("%" PRIuLEAST32) >= 3, "PRI LEAST32 o/u");
_Static_assert(sizeof("%" PRIxLEAST32) >= 3 && sizeof("%" PRIXLEAST32) >= 3, "PRI LEAST32 x/X");
_Static_assert(sizeof("%" PRIdFAST32) >= 3 && sizeof("%" PRIiFAST32) >= 3, "PRI FAST32 d/i");
_Static_assert(sizeof("%" PRIoFAST32) >= 3 && sizeof("%" PRIuFAST32) >= 3, "PRI FAST32 o/u");
_Static_assert(sizeof("%" PRIxFAST32) >= 3 && sizeof("%" PRIXFAST32) >= 3, "PRI FAST32 x/X");

/* MAX / PTR variants. */
_Static_assert(sizeof("%" PRIdMAX) >= 3 && sizeof("%" PRIiMAX) >= 3, "PRI MAX d/i");
_Static_assert(sizeof("%" PRIoMAX) >= 3 && sizeof("%" PRIuMAX) >= 3, "PRI MAX o/u");
_Static_assert(sizeof("%" PRIxMAX) >= 3 && sizeof("%" PRIXMAX) >= 3, "PRI MAX x/X");
_Static_assert(sizeof("%" PRIdPTR) >= 3 && sizeof("%" PRIiPTR) >= 3, "PRI PTR d/i");
_Static_assert(sizeof("%" PRIoPTR) >= 3 && sizeof("%" PRIuPTR) >= 3, "PRI PTR o/u");
_Static_assert(sizeof("%" PRIxPTR) >= 3 && sizeof("%" PRIXPTR) >= 3, "PRI PTR x/X");

int main(void) { return 0; }
