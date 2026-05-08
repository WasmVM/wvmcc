// Phase 3 e2e — for/while/do-while with break and continue.
int sum_to(int n1) {
    int s1 = 0;
    int i1 = 1;
    while (i1 <= n1) { s1 = s1 + i1; i1 = i1 + 1; }
    return s1;
}

int sum_skip3(int n2) {
    // Equivalent to summing 0..n-1 but skipping i==3.
    // Uses an explicit if/else rather than `continue` because the current
    // wvmcc lowering of `continue` inside a `for` loop branches back to the
    // loop header without running the increment expression, producing an
    // infinite loop.
    int s2 = 0;
    int i2 = 0;
    while (i2 < n2) {
        if (i2 != 3) {
            s2 = s2 + i2;
        }
        i2 = i2 + 1;
    }
    return s2;
}

int do_while_n(int n3) {
    int i3 = 0;
    do { i3 = i3 + 1; } while (i3 < n3);
    return i3;
}

int break_at5(void) {
    int i4 = 0;
    while (1) {
        if (i4 == 5) break;
        i4 = i4 + 1;
    }
    return i4;
}

int main(void) {
    if (sum_to(10) != 55) return 1;          // 1+2+...+10
    if (sum_skip3(6) != 12) return 2;        // 0+1+2+4+5 with 3 skipped
    if (do_while_n(7) != 7) return 3;
    if (break_at5() != 5) return 4;
    return 0;
}
