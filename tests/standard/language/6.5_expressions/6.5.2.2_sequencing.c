/* LANG-6.5.2.2-10 — there is a sequence point after the evaluation of the
 * function designator and arguments and before the call; the callee's execution
 * is indeterminately sequenced with respect to other evaluations in the caller
 * but is not interleaved with them (6.5.2.2p10). Verify=exit; 0 on pass. */
static int log_idx;
static int order[8];

static int mark(int id) { order[log_idx++] = id; return 0; }

/* All side effects of evaluating the arguments must complete before the call's
 * own body runs (sequence point before the call). */
static int callee(int a, int b) {
    (void)a; (void)b;
    return mark(99);   /* recorded only after both argument marks */
}

int main(void) {
    log_idx = 0;
    callee(mark(1), mark(2));
    /* Argument evaluations (1 and 2, in some order) precede the body mark (99). */
    if (log_idx != 3) return 1;
    if (order[2] != 99) return 2;            /* body ran last */
    int a0 = order[0], a1 = order[1];
    if (!((a0 == 1 && a1 == 2) || (a0 == 2 && a1 == 1))) return 3;
    return 0;
}
