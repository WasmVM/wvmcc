// M2-B follow-up — variadic float args (default promotion float→double, spilled
// as i64 via reinterpret). The M2-A spill path now handles f32/f64.

#define va_list            __builtin_va_list
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, T)      __builtin_va_arg(ap, T)
#define va_end(ap)         __builtin_va_end(ap)

double sum_d(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double total = 0.0;
    for (int i = 0; i < n; i = i + 1) {
        total = total + va_arg(ap, double);
    }
    va_end(ap);
    return total;
}

int main(void) {
    // 1.5 + 2.5 + 3.0 = 7.0 → (int)7 == 7
    double s = sum_d(3, 1.5, 2.5, 3.0);
    if ((int)s != 7) return 1;

    // Float promoted to double in variadic context: 3.14f passed as double.
    double s2 = sum_d(1, 3.14f);
    if ((int)(s2 * 100.0) != 314) return 2;

    return 0;
}
