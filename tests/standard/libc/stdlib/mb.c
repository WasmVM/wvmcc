/* tests/standard/libc/stdlib/mb.c — LIBC-stdlib-mb-01 (C17 7.22.7, 7.22.8).
 * mblen / mbtowc / wctomb / mbstowcs / wcstombs convert between multibyte
 * and wide characters. ASCII-only inputs so the test is valid in the "C"
 * locale regardless of MB_CUR_MAX. Verify=exit. */
#include <stdlib.h>

int main(void) {
    char buf[8];
    wchar_t wc = 0;
    wchar_t wbuf[4];
    if (mblen("A", 1) != 1) return 1;
    if (mblen("", 1) != 0) return 2;            /* null character -> 0 */
    if (mbtowc(&wc, "B", 1) != 1) return 3;
    if (wc != L'B') return 4;
    if (wctomb(buf, L'C') != 1) return 5;
    if (buf[0] != 'C') return 6;
    if (mbstowcs(wbuf, "hi", 4) != 2) return 7;
    if (wbuf[0] != L'h' || wbuf[1] != L'i' || wbuf[2] != L'\0') return 8;
    if (wcstombs(buf, wbuf, 8) != 2) return 9;
    if (buf[0] != 'h' || buf[1] != 'i' || buf[2] != '\0') return 10;
    return 0;
}
