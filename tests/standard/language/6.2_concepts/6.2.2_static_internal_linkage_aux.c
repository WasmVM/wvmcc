/* LANG-6.2.2-01 (companion TU) — 6.2.2p3: this TU has its OWN `static int
 * counter` with internal linkage, distinct from the one in
 * 6.2.2_static_internal_linkage.c. See that file for the test driver.
 */

static int counter = 99;          /* internal linkage: private to this TU */

int aux_counter(void) {
    return counter;
}

int aux_counter_is_distinct(int *this_tu_counter) {
    return this_tu_counter != &counter;
}
