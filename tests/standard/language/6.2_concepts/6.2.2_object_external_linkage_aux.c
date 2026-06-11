/* LANG-6.2.2-03 (companion TU) — 6.2.2p5: references the externally-linked
 * file-scope object `shared` defined in 6.2.2_object_external_linkage.c. Both
 * names denote the same object. See that file for the test driver.
 */

extern int shared;                /* same object as in the other TU */

int aux_read_shared(void) {
    return shared;
}

void aux_set_shared(int v) {
    shared = v;
}
