// Phase 2 e2e — struct field read/write through pointers.
//
// Uses a `static` local struct (lives in mem[0] via the static data
// allocator) instead of an automatic local. Automatic struct allocation
// currently overshoots the shadow-stack frame in wvmcc (the frame-pointer
// local holds the saved SP rather than the new frame base — separate bug).
struct Point { int x; int y; };

void write_point(struct Point *p, int x, int y) { p->x = x; p->y = y; }
int read_x(struct Point *p) { return p->x; }
int read_y(struct Point *p) { return p->y; }
int sum_point(struct Point *p) { return p->x + p->y; }

int main(void) {
    static struct Point p;
    write_point(&p, 3, 7);
    if (read_x(&p) != 3) return 1;
    if (read_y(&p) != 7) return 2;
    if (sum_point(&p) != 10) return 3;
    return 0;
}
