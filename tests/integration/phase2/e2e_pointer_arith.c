// Phase 2 e2e — pointer arithmetic via int* indexing.
//
// Uses a static struct as the storage source rather than an int array:
// static int arrays and global arrays both currently lower to addresses the
// codegen doesn't follow correctly (separate compiler bugs), while
// static-struct addressing works.
struct Vec3 { int a; int b; int c; };

void set_vec(struct Vec3 *p, int a, int b, int c) {
    p->a = a; p->b = b; p->c = c;
}

int load_at(int *arr, int idx) { return arr[idx]; }
int load_next(int *p) { return *(p + 1); }

int main(void) {
    static struct Vec3 v;
    set_vec(&v, 10, 20, 30);
    int *base = &v.a;
    if (load_at(base, 0) != 10) return 1;
    if (load_at(base, 1) != 20) return 2;
    if (load_at(base, 2) != 30) return 3;
    if (load_next(base) != 20) return 4;
    return 0;
}
