// #78 regression — taking the address of an automatic local and dereferencing
// it through an opaque pointer must reach the right linear memory.
//
// wvmcc uses two linear memories: mem[0] = static data + heap, mem[1] = the
// shadow stack (automatic / address-taken locals). A pointer value carries its
// target memory in a 4-bit tag in the high nibble (bit 60); opaque pointer
// derefs (`*p`, `p->m`, `p[i]`) dispatch on that tag at runtime. Before that
// scheme landed, every deref hit mem[0], so writes through `&local` were lost
// and each check below returned the wrong value.
//
// Returns 0 iff every case observes the value written through the pointer.

struct S { int x; };

static void set_struct(struct S *p) { p->x = 5; }   // p->m  (Dynamic deref)
static void set_int(int *p)         { *p = 9; }      // *p
static void set_arr(int *p)         { p[0] = 7; }    // p[i]

// Same callee invoked with a stack pointer (mem[1]) and a static pointer
// (mem[0]): proves the deref dispatches per-pointer at runtime rather than
// being fixed to one memory at compile time.
static int g_static;
static void set_six(int *p)         { *p = 6; }

int main(void) {
    struct S s; s.x = 0; set_struct(&s);
    if (s.x != 5) return 1;

    int v = 0; set_int(&v);
    if (v != 9) return 2;

    int a[2]; a[0] = 0; set_arr(a);
    if (a[0] != 7) return 3;

    // Mixed dispatch: &local (mem[1]) then &static (mem[0]) through one callee.
    int local = 0;
    set_six(&local);
    set_six(&g_static);
    if (local != 6) return 4;
    if (g_static != 6) return 5;

    // Address of a local taken into a local pointer var, dereffed in-function.
    int w = 0; int *pw = &w; *pw = 8;
    if (w != 8) return 6;

    return 0;
}
