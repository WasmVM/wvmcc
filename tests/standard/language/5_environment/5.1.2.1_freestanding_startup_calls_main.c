/* LANG-5.1.2.1-01 — 5.1.2.1p1: in a freestanding environment the name and
 * type of the startup function are implementation-defined. wvmcc (docs/spec.md)
 * uses a crt0 start-wrapper that calls `main`; WasmVM invokes the module start
 * function. This program verifies that startup actually reaches `main` with
 * static objects initialized. */
static int initialized = 42;    /* static storage initialized before startup */

int main(void)
{
    if (initialized != 42) return 1;
    return 0;
}
