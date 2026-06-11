/* LANG-6.2.2-03 — 6.2.2p5: a file-scope object declared with no storage-class
 * specifier has external linkage, so the same name in another translation unit
 * denotes the SAME object (one shared definition across the program).
 *
 * Two-TU link test. The companion TU is 6.2.2_object_external_linkage_aux.c,
 * which references this `shared` object via `extern` and mutates it.
 */

int shared = 5;                   /* no SCS at file scope -> external linkage */

/* Defined in the companion TU; reads/writes the SAME `shared` object. */
extern int aux_read_shared(void);
extern void aux_set_shared(int v);

int main(void) {
    if (shared != 5) return 1;

    /* The aux TU sees our write (same object). */
    shared = 17;
    if (aux_read_shared() != 17) return 2;

    /* Our TU sees the aux TU's write (same object). */
    aux_set_shared(123);
    if (shared != 123) return 3;

    return 0;
}
