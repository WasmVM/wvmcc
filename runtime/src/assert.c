// M2-15: assert failure handler.
//
// stdio (M2-12, M2-13) isn't required yet — we write the failure
// banner directly through unistd.write(STDERR_FILENO, ...) and abort.
// A nicer formatted line will land once printf is in.

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static void emit(const char *s) {
    if (s) write(2, s, strlen(s));
}

// Minimal int→decimal (printf is not a dependency of <assert.h>).
static void emit_int(int v) {
    char buf[12];
    int i = (int)sizeof buf;
    unsigned u = v < 0 ? (unsigned)(-(long)v) : (unsigned)v;
    buf[--i] = '\0';
    do { buf[--i] = (char)('0' + u % 10); u /= 10; } while (u);
    if (v < 0) buf[--i] = '-';
    emit(&buf[i]);
}

_Noreturn void __wvmcc_assert_fail(const char *expr, const char *file, int line, const char *func) {
    emit("Assertion failed: ");
    emit(expr);
    emit(", function ");
    emit(func);
    emit(", file ");
    emit(file);
    emit(", line ");
    emit_int(line);
    emit("\n");
    abort();
}
