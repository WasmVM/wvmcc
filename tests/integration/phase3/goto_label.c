// Phase 3 verification: forward goto with label.
// Expected: forward goto lifts into a wrapping block; br to its end skips the
// intervening statements. Goto and label must be at the same compound level.

int simple_forward(void) {
    int s = 5;
    goto end1;
    s = 99;     // skipped
end1:
    return s;
}

int two_forwards(void) {
    int t = 0;
    goto a;
    t = 1;
a:
    t = t + 10;
    goto b;
    t = 2;
b:
    return t;
}
