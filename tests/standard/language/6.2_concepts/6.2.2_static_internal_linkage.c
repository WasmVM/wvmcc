/* LANG-6.2.2-01 — 6.2.2p3: a file-scope identifier declared with `static`
 * has internal linkage, so it denotes a distinct object/function private to
 * this translation unit; the same name in another TU refers to something else.
 *
 * Two-TU link test. The companion TU is 6.2.2_static_internal_linkage_aux.c,
 * which defines its own `static int counter` and `static int aux_val(void)`,
 * plus an external `int aux_counter_addr_differs(void)` that reports whether
 * the aux TU's private `counter` is a different object from this TU's.
 */

static int counter = 11;          /* internal linkage: private to this TU */

static int local_val(void) {      /* internal linkage: private to this TU */
    return 22;
}

/* Defined in the companion TU; reads ITS private `counter`. */
extern int aux_counter(void);
/* Defined in the companion TU; returns 1 iff this TU's `counter` (passed by
 * address) is a different object than the aux TU's private `counter`. */
extern int aux_counter_is_distinct(int *this_tu_counter);

int main(void) {
    if (counter != 11) return 1;
    if (local_val() != 22) return 2;

    /* The aux TU's private `counter` is a separate object initialised to 99. */
    if (aux_counter() != 99) return 3;

    /* Mutating our private object must not be visible through the aux TU's. */
    counter = 12345;
    if (aux_counter() != 99) return 4;

    /* The two `counter`s are genuinely distinct objects. */
    if (!aux_counter_is_distinct(&counter)) return 5;

    return 0;
}
