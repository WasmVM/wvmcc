/* tests/standard/libc/string/strcoll.c — LIBC-string-strcoll-01. ISO C17 §7.24.4.3. Verify=exit.
 * strcoll compares per the LC_COLLATE category; in the default "C" locale the
 * ordering matches strcmp. Sign only. */
#include <string.h>
int main(void) {
    if (strcoll("abc", "abc") != 0) return 1;
    if (!(strcoll("a", "b") < 0)) return 2;
    if (!(strcoll("b", "a") > 0)) return 3;
    if (!(strcoll("ab", "abc") < 0)) return 4;
    return 0;
}
