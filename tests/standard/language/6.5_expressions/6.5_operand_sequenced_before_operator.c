/* LANG-6.5-01 — 6.5p1: "The value computations of the operands of an operator
 * are sequenced before the value computation of the result of the operator."
 * The operand value computations must be complete (and any function calls in
 * them returned) before the operator combines them. */

static int log_index = 0;
static int order[4];

static int rec(int v) {
    order[log_index++] = v;
    return v;
}

int main(void) {
    /* Both operand calls must complete before the '+' produces its result. */
    int sum = rec(3) + rec(4);
    if (sum != 7) return 1;

    /* Two operand value computations both ran, recording two entries. */
    if (log_index != 2) return 2;
    if (order[0] != 3) return 3;
    if (order[1] != 4) return 4;

    /* Nested: the inner operator's result is sequenced before the outer one.
     * (a*b) must be a finished value before it is added to c. */
    int a = 2, b = 5, c = 1;
    int r = a * b + c;
    if (r != 11) return 5;

    return 0;
}
