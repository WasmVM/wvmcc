/* LANG-4-02 — 4p4: a #error directive that appears in a group skipped by
 * conditional inclusion is not processed, so translation succeeds (6.10.1p6:
 * directives within a skipped group are not processed except for nesting
 * of conditionals). */

#if 0
#error "this directive is in a skipped group and must NOT fail translation"
#endif

#ifdef NEVER_DEFINED_LANG_4_02
#error "also skipped: group controlled by an undefined macro"
#endif

int main(void)
{
    /* If translation reached here, the skipped #error directives were
     * correctly ignored. */
    return 0;
}
