/* tests/standard/libc/inttypes/strtoimax.c — LIBC-inttypes-strtoimax-01.
 * ISO C17 §7.8.2.3: strtoimax/strtoumax parse greatest-width integers,
 * equivalent to strtol/strtoul except for the result type. Covers decimal,
 * sign, endptr positioning, base 0 (hex prefix), and explicit base 16.
 * Catalog status: partial. Verify=exit. Kept small for WasmVM. */
#include <inttypes.h>

int main(void) {
    char *end;

    /* Plain decimal; endptr lands on the terminating NUL. */
    if (strtoimax("123", &end, 10) != 123) return 1;
    if (*end != '\0') return 2;

    /* Leading whitespace + sign; endptr lands on first unparsed char. */
    if (strtoimax(" -42x", &end, 10) != -42) return 3;
    if (*end != 'x') return 4;

    /* Base 0 auto-detects the 0x prefix. */
    if (strtoimax("0x10", &end, 0) != 16) return 5;
    if (*end != '\0') return 6;

    /* No conversion: returns 0 and endptr == nptr. */
    {
        const char *s = "q9";
        if (strtoimax(s, &end, 10) != 0) return 7;
        if (end != (char *)s) return 8;
    }

    /* strtoumax, explicit base 16. */
    if (strtoumax("ff", &end, 16) != 255) return 9;
    if (*end != '\0') return 10;

    /* strtoumax negates into unsigned range: -1 -> UINTMAX_MAX. */
    if (strtoumax("-1", &end, 10) != UINTMAX_MAX) return 11;

    return 0;
}
