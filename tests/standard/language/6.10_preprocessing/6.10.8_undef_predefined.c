/* LANG-6.10.8-04 (#undef variant) — constraint violation (C17 6.10.8p2):
 * predefined macros shall not be the subject of a #undef directive either. */
#undef __FILE__

int main(void) {
    return 0;
}
