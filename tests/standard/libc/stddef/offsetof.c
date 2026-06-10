/* tests/standard/libc/stddef/offsetof.c — LIBC-stddef-offsetof-01 (C17 7.19p3).
 * Verify=exit. offsetof(type, member-designator) expands to an integer
 * constant expression of type size_t, the offset in bytes of the member
 * from the beginning of its structure. Verified at runtime: wvmcc's
 * offsetof is the null-pointer trick (address constant, not an ICE). */
#include <stddef.h>

struct s {
    char a;
    long b;
    char c;
};

int main(void) {
    struct s v;
    /* The first member has offset 0 (6.7.2.1p15). */
    if (offsetof(struct s, a) != 0) return 1;
    /* offsetof must agree with actual member addresses. */
    if ((size_t)((char *)&v.b - (char *)&v) != offsetof(struct s, b)) return 2;
    if ((size_t)((char *)&v.c - (char *)&v) != offsetof(struct s, c)) return 3;
    /* Members have increasing addresses (6.7.2.1p15). */
    if (offsetof(struct s, b) >= offsetof(struct s, c)) return 4;
    return 0;
}
