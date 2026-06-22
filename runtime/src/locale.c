// <locale.h> (C17 7.11). wvmcc supports only the "C" locale, so setlocale
// honors only "C" / "" (the native environment, which is "C" here) and
// localeconv reports the fixed "C"-locale values of 7.11.2.1p4.

#include <locale.h>
#include <limits.h>

// File-scope static data (function-local static initialized arrays don't yet
// relocate correctly when linked from libc.a; file-scope ones do).
static char __c_name[] = "C";
static char __dot[]    = ".";
static char __empty[]  = "";
static struct lconv __c_lconv;

char *setlocale(int category, const char *locale) {
    (void)category;
    // A null `locale` queries the current locale (always "C").
    if (locale == (const char *)0) return __c_name;
    // "C" and "" (native environment) shall be honored (7.11.1.1p3).
    if (locale[0] == '\0') return __c_name;
    if (locale[0] == 'C' && locale[1] == '\0') return __c_name;
    // No other locale can be selected.
    return (char *)0;
}

struct lconv *localeconv(void) {
    struct lconv *c = &__c_lconv;
    c->decimal_point     = __dot;
    c->thousands_sep     = __empty;
    c->grouping          = __empty;
    c->mon_decimal_point = __empty;
    c->mon_thousands_sep = __empty;
    c->mon_grouping      = __empty;
    c->positive_sign     = __empty;
    c->negative_sign     = __empty;
    c->currency_symbol   = __empty;
    c->int_curr_symbol   = __empty;
    // All char members are CHAR_MAX in the "C" locale (7.11.2.1p4).
    c->frac_digits        = CHAR_MAX;
    c->p_cs_precedes      = CHAR_MAX;
    c->n_cs_precedes      = CHAR_MAX;
    c->p_sep_by_space     = CHAR_MAX;
    c->n_sep_by_space     = CHAR_MAX;
    c->p_sign_posn        = CHAR_MAX;
    c->n_sign_posn        = CHAR_MAX;
    c->int_frac_digits    = CHAR_MAX;
    c->int_p_cs_precedes  = CHAR_MAX;
    c->int_n_cs_precedes  = CHAR_MAX;
    c->int_p_sep_by_space = CHAR_MAX;
    c->int_n_sep_by_space = CHAR_MAX;
    c->int_p_sign_posn    = CHAR_MAX;
    c->int_n_sign_posn    = CHAR_MAX;
    return c;
}
