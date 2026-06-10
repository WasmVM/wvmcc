/* LANG-4-01 — 4p4: the implementation shall produce at least one diagnostic
 * message (and fail translation) if a preprocessing translation unit contains
 * a #error preprocessing directive in a group that is not skipped by
 * conditional inclusion (6.10.5). */

/* This #error is in a non-skipped group, so a conforming implementation MUST
 * reject this translation unit. */
#if 1
#error "LANG-4-01: #error in a non-skipped group must fail translation"
#endif

int main(void) { return 0; }
