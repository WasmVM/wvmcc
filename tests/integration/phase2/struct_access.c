// Phase 2 verification: struct field read/write
// Expected: shadow-stack allocation, field stores at correct offsets
struct Point { int x; int y; };

void write_point(struct Point *p, int x, int y) {
    p->x = x;
    p->y = y;
}

int read_x(struct Point *p) {
    return p->x;
}

int read_y(struct Point *p) {
    return p->y;
}

int sum_point(struct Point *p) {
    return p->x + p->y;
}
