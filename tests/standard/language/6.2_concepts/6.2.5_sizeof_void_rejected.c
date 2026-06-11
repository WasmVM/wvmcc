/* LANG-6.2.5-12 — 6.2.5p19: void is an incomplete object type that cannot be
 * completed. Applying sizeof to void (an incomplete type) is a constraint
 * violation (6.5.3.4p1) and must be rejected by a conforming compiler. */

unsigned long bad = sizeof(void);
