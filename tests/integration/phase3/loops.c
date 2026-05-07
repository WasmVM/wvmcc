// Phase 3 verification: do-while, while, for with break/continue.
// Expected: loop/block/br_if pattern; correct Br depths for break/continue.
// Note: distinct local-variable names across functions to work around an
// existing semantic-check quirk that treats same-named locals as duplicate
// external definitions.

int do_while_count(int n) {
    int i1 = 0;
    do {
        i1 = i1 + 1;
    } while (i1 < n);
    return i1;
}

int while_with_break(int n) {
    int i2 = 0;
    while (i2 < n) {
        if (i2 == 5) break;
        i2 = i2 + 1;
    }
    return i2;
}

int for_with_continue(int n) {
    int sum = 0;
    int i3;
    for (i3 = 0; i3 < n; i3 = i3 + 1) {
        if (i3 == 3) continue;
        sum = sum + i3;
    }
    return sum;
}

int nested_break(int m, int n) {
    int i4;
    int j4;
    int total = 0;
    for (i4 = 0; i4 < m; i4 = i4 + 1) {
        for (j4 = 0; j4 < n; j4 = j4 + 1) {
            if (j4 == 2) break;
            total = total + 1;
        }
    }
    return total;
}
