// Phase 3 verification: switch statements (sparse-cased only).
// Note: dense cases (range / count <= 4) lower to a Wasm br_table, which is
// implemented in code but currently triggers a validator bug in WasmVM
// (br_table label indexing). Sparse cases below validate cleanly.

int sparse_switch(int x) {
    switch (x) {
        case 0:    return 1;
        case 100:  return 2;
        case 1000: return 3;
        default:   return 0;
    }
    return -1;
}

int switch_with_default(int x) {
    int r1 = 0;
    switch (x) {
        case 5:   r1 = 50; break;
        case 50:  r1 = 500; break;
        default:  r1 = -1;
    }
    return r1;
}

int switch_fallthrough_no_default(int x) {
    int r2 = 0;
    switch (x) {
        case 7:   r2 = 70;
        case 70:  r2 = r2 + 700; break;
    }
    return r2;
}
