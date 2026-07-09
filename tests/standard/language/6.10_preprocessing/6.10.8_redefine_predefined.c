/* LANG-6.10.8-04 — constraint violation (C17 6.10.8p2): none of the
 * predefined macro names (nor __LINE__/__FILE__, nor `defined`) shall be
 * the subject of a #define or #undef directive. */
#define __LINE__ 5

int main(void) {
    return 0;
}
