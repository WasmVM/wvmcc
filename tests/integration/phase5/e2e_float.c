// M2-B e2e — float/double arithmetic, comparisons, casts, and union bit access.
//
// Structured so all assertions fall through to `return 0` on success (avoids
// triggering an unrelated WasmVM bug with if-branches that contain return).

int main(void) {
    // Basic double arithmetic and float→int truncation.
    double x = 1.5;
    double y = x * 2.0 + 3.14;
    if ((int)y != 6) return 1;

    // float type, multiplication, suffixed literal.
    float f = 3.14f;
    float g = f * f;
    if ((int)g != 9) return 2;

    // Float comparisons.
    double a = 2.5;
    double b = 1.5;
    if (!(a > b))   return 3;
    if (a < b)      return 4;
    if (a == b)     return 5;
    if (!(a != b))  return 6;
    if (!(a >= a))  return 7;
    if (!(a <= a))  return 8;

    // Division.
    double d = 10.0 / 4.0;
    if ((int)(d * 100.0) != 250) return 9;

    // Subtraction.
    if ((int)(5.5 - 1.5) != 4) return 10;

    // IEEE 754 bit pattern via union: exponent bias of 1.0 is 1023.
    union { double d; unsigned long u; } u;
    u.d = 1.0;
    if ((u.u >> 52) != 1023) return 11;

    return 0;
}
