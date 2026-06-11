/* tests/standard/libc/stdlib/getenv.c — LIBC-stdlib-getenv-01 (C17 7.22.4.6).
 * getenv looks up a name in the environment list; if the name is not found
 * it returns a null pointer (7.22.4.6p4). Verify=exit. */
#include <stdlib.h>

int main(void) {
    /* A name no environment defines: the lookup must fail with NULL. */
    if (getenv("WVMCC_STD_GETENV_UNSET_7_22_4_6") != NULL) return 1;
    return 0;
}
