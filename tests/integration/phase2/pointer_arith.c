// Phase 2 verification: pointer arithmetic and array indexing
void store_int(int *p, int v) {
    *p = v;
}

int load_int(int *p) {
    return *p;
}

// Verify pointer arithmetic: arr[i] = *(arr + i*4)
int ptr_offset(int *arr, int idx) {
    return arr[idx];
}

// Verify pointer increment
int ptr_next(int *p) {
    return *(p + 1);
}
