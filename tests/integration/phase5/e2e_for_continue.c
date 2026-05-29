// Regression: `continue` in a `for` loop must execute the loop's step before
// re-testing the condition. An earlier codegen branched `continue` to the loop
// top, skipping the step — any counting loop using `continue` hung forever.
// Uses int (i32) throughout to dodge the WasmVM i64-compare interpreter bug.

int main(void) {
    int sum = 0;
    for (int i = 0; i < 6; i++) {
        if (i == 2) continue;   // skip 2; step must still run or this hangs
        if (i == 4) continue;   // skip 4
        sum += i;               // 0+1+3+5 = 9
    }
    // nested loops with continue in the inner one
    int prod = 0;
    for (int a = 0; a < 3; a++) {
        for (int b = 0; b < 3; b++) {
            if (b == 1) continue;   // count b in {0,2}
            prod += 1;
        }
    }
    // prod = 3 outer * 2 inner = 6
    return (sum == 9 && prod == 6) ? 0 : 1;
}
