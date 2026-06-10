/* tests/standard/libc/locale/setlocale.c — LIBC-locale-setlocale-01
 * (C17 7.11.1.1). Verify=exit.
 *
 * At program startup the equivalent of setlocale(LC_ALL, "C") is executed
 * (7.11.1.1p4). setlocale returns a non-null string on success and NULL only
 * if the request cannot be honored (7.11.1.1p7-8); "C" and "" shall be
 * honored (7.11.1.1p3). A NULL locale argument queries the current locale
 * without changing it. Kept small for the WasmVM interpreter. */
#include <locale.h>

int main(void) {
    /* Query the startup locale: must return a non-null string. */
    if (setlocale(LC_ALL, (const char *)0) == (char *)0) return 1;
    /* Selecting the "C" locale shall succeed. */
    if (setlocale(LC_ALL, "C") == (char *)0) return 2;
    /* Selecting the native environment ("") shall succeed. */
    if (setlocale(LC_ALL, "") == (char *)0) return 3;
    /* A single category may be selected independently. */
    if (setlocale(LC_NUMERIC, "C") == (char *)0) return 4;
    /* Query a single category. */
    if (setlocale(LC_CTYPE, (const char *)0) == (char *)0) return 5;
    return 0;
}
