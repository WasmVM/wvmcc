/* LANG-6.9.2-04 — a tentative definition with internal linkage shall not have an
 * incomplete type (6.9.2p3). `struct never` is never completed. */
static struct never s;
