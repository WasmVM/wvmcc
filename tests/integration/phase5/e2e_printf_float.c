// M2-14 e2e — float/double printf via %f, %e, %g, %a. Exits 0 iff every
// conversion matches the expected output. Each block uses snprintf into
// a fixed buffer so the entire suite stays in one TU and we don't have
// to capture stdout from multiple lines.
#include <stdio.h>
#include <string.h>

static int eq(const char *got, const char *want) {
    return strcmp(got, want) == 0;
}

int main(void) {
    char buf[64];

    // Acceptance criteria from issue #71 ----------------------------------
    snprintf(buf, sizeof(buf), "%.5f", 3.14159);
    if (!eq(buf, "3.14159")) return 1;

    snprintf(buf, sizeof(buf), "%e", 1.5e10);
    if (!eq(buf, "1.500000e+10")) return 2;

    snprintf(buf, sizeof(buf), "%g", 1.5e10);
    if (!eq(buf, "1.5e+10")) return 3;

    snprintf(buf, sizeof(buf), "%g", 1.5);
    if (!eq(buf, "1.5")) return 4;

    snprintf(buf, sizeof(buf), "%a", 1.0);
    if (!eq(buf, "0x1.0p+0")) return 5;

    snprintf(buf, sizeof(buf), "%f", 0.1 + 0.2);
    if (!eq(buf, "0.300000")) return 6;

    snprintf(buf, sizeof(buf), "%f", -0.0);
    if (!eq(buf, "-0.000000")) return 7;

    // Extra coverage ------------------------------------------------------
    snprintf(buf, sizeof(buf), "%a", 1.5);
    if (!eq(buf, "0x1.8p+0")) return 8;

    snprintf(buf, sizeof(buf), "%a", 2.0);
    if (!eq(buf, "0x1.0p+1")) return 9;

    snprintf(buf, sizeof(buf), "%E", 1.5e10);
    if (!eq(buf, "1.500000E+10")) return 10;

    snprintf(buf, sizeof(buf), "%.0f", 3.7);
    if (!eq(buf, "4")) return 11;

    snprintf(buf, sizeof(buf), "%.2f", 1.0 / 3.0);
    if (!eq(buf, "0.33")) return 12;

    snprintf(buf, sizeof(buf), "%g", 0.0001);
    if (!eq(buf, "0.0001")) return 13;

    snprintf(buf, sizeof(buf), "%g", 0.00001);  // < 1e-4 → %e form
    if (!eq(buf, "1e-05")) return 14;

    return 0;
}
