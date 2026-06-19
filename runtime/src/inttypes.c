// <inttypes.h> (C17 7.8.2) — greatest-width integer arithmetic and the
// numeric conversions strtoimax/strtoumax. In wvmcc's LP64 model intmax_t is
// `long` and uintmax_t is `unsigned long`, so the conversions are exactly
// strtol/strtoul.

#include <inttypes.h>
#include <stdlib.h>

intmax_t imaxabs(intmax_t j) {
    return j < 0 ? -j : j;
}

imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom) {
    // C division truncates toward zero (6.5.5), so quot*denom + rem == numer.
    imaxdiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

intmax_t strtoimax(const char *restrict nptr, char **restrict endptr, int base) {
    return strtol(nptr, endptr, base);
}

uintmax_t strtoumax(const char *restrict nptr, char **restrict endptr, int base) {
    return strtoul(nptr, endptr, base);
}
