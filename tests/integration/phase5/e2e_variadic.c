// M2-A e2e — variadic function support via __builtin_va_*.
//
// <stdarg.h> arrives in M2-1; until then, the test wires the builtins
// directly with local macros so the variadic ABI itself is what's exercised.

// Use macros (not typedef) so the parser sees __builtin_va_list directly —
// typedef-name resolution doesn't currently flow through to the codegen type
// system, which would mis-size `ap` as i32. M2-1 ships a real <stdarg.h>.
#define va_list            __builtin_va_list
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, T)      __builtin_va_arg(ap, T)
#define va_end(ap)         __builtin_va_end(ap)

int sum(int n, ...) {
    va_list ap;
    va_start(ap, n);
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        total = total + va_arg(ap, int);
    }
    va_end(ap);
    return total;
}

int main(void) {
    if (sum(3, 10, 20, 30) != 60) return 1;
    if (sum(1, 42) != 42)         return 2;
    if (sum(0) != 0)              return 3;
    return 0;
}
