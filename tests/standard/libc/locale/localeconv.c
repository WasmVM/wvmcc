/* tests/standard/libc/locale/localeconv.c — LIBC-locale-localeconv-01
 * (C17 7.11.2.1). Verify=exit.
 *
 * In the "C" locale (in effect at startup, 7.11.1.1p4) the members of the
 * object returned by localeconv() shall have the values given in 7.11.2.1p4:
 * decimal_point is ".", the other string members are "", and the char
 * members are CHAR_MAX. Byte comparisons keep the call graph tiny for the
 * WasmVM interpreter (no strcmp). */
#include <locale.h>
#include <limits.h>

int main(void) {
    struct lconv *lc = localeconv();
    if (lc == (struct lconv *)0) return 1;
    /* decimal_point == "." */
    if (lc->decimal_point[0] != '.' || lc->decimal_point[1] != '\0') return 2;
    /* thousands_sep == "" */
    if (lc->thousands_sep[0] != '\0') return 3;
    /* grouping == "" */
    if (lc->grouping[0] != '\0') return 4;
    /* mon_decimal_point == "" */
    if (lc->mon_decimal_point[0] != '\0') return 5;
    /* currency_symbol == "" */
    if (lc->currency_symbol[0] != '\0') return 6;
    /* int_curr_symbol == "" */
    if (lc->int_curr_symbol[0] != '\0') return 7;
    /* char members are CHAR_MAX in the "C" locale */
    if (lc->frac_digits != CHAR_MAX) return 8;
    if (lc->p_cs_precedes != CHAR_MAX) return 9;
    if (lc->n_sign_posn != CHAR_MAX) return 10;
    return 0;
}
