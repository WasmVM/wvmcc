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

_Noreturn void __wvmcc_assert_fail(const char *expr, const char *file, int line, const char *func) {
    emit("Assertion failed: ");
    emit(expr);
    emit(", function ");
    emit(func);
    emit(", file ");
    emit(file);
    /* line number is left out for now — printf isn't available yet
       (M2-13). A follow-up will swap this for fprintf and include the
       line. */
    (void)line;
    emit("\n");
    abort();
}
