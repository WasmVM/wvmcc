/* tests/standard/libc/signal/types.c — <signal.h> types and macros.
 * Catalog: LIBC-signal-types-01 (docs/standard/libc.md). C17 §7.14.
 * Verify=static-assert (compile-only, -ffreestanding).
 *
 * §7.14p2: sig_atomic_t is a (possibly volatile-qualified) integer type that
 * can be accessed as an atomic entity even in the presence of asynchronous
 * interrupts.
 * §7.14p3: SIG_DFL, SIG_ERR, SIG_IGN expand to constant expressions with
 * distinct values, of type compatible with the second argument to / return
 * value of signal() — i.e. void (*)(int).
 * §7.14p4: SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM expand to
 * positive integer constant expressions with distinct values. */
#include <signal.h>

/* --- presence: every entity must be defined as a macro (the names below,
 * other than sig_atomic_t, are required to be macros, §7.14p3-4) --- */
#ifndef SIG_DFL
#error "SIG_DFL not defined"
#endif
#ifndef SIG_ERR
#error "SIG_ERR not defined"
#endif
#ifndef SIG_IGN
#error "SIG_IGN not defined"
#endif
#ifndef SIGABRT
#error "SIGABRT not defined"
#endif
#ifndef SIGFPE
#error "SIGFPE not defined"
#endif
#ifndef SIGILL
#error "SIGILL not defined"
#endif
#ifndef SIGINT
#error "SIGINT not defined"
#endif
#ifndef SIGSEGV
#error "SIGSEGV not defined"
#endif
#ifndef SIGTERM
#error "SIGTERM not defined"
#endif

/* --- sig_atomic_t: must name an integer type usable for a file-scope
 * volatile object (the canonical signal-flag idiom) --- */
static volatile sig_atomic_t sig_flag = 0;

/* --- SIG_DFL / SIG_ERR / SIG_IGN: constant expressions usable as static
 * initializers of a signal-handler pointer (their values are not integer
 * constant expressions, so distinctness is not _Static_assert-able) --- */
static void (*const handler_dfl)(int) = SIG_DFL;
static void (*const handler_err)(int) = SIG_ERR;
static void (*const handler_ign)(int) = SIG_IGN;

/* --- signal numbers: positive integer constant expressions --- */
_Static_assert(SIGABRT > 0, "SIGABRT is a positive integer constant");
_Static_assert(SIGFPE > 0, "SIGFPE is a positive integer constant");
_Static_assert(SIGILL > 0, "SIGILL is a positive integer constant");
_Static_assert(SIGINT > 0, "SIGINT is a positive integer constant");
_Static_assert(SIGSEGV > 0, "SIGSEGV is a positive integer constant");
_Static_assert(SIGTERM > 0, "SIGTERM is a positive integer constant");

/* --- signal numbers: pairwise distinct values --- */
_Static_assert(SIGABRT != SIGFPE, "SIGABRT != SIGFPE");
_Static_assert(SIGABRT != SIGILL, "SIGABRT != SIGILL");
_Static_assert(SIGABRT != SIGINT, "SIGABRT != SIGINT");
_Static_assert(SIGABRT != SIGSEGV, "SIGABRT != SIGSEGV");
_Static_assert(SIGABRT != SIGTERM, "SIGABRT != SIGTERM");
_Static_assert(SIGFPE != SIGILL, "SIGFPE != SIGILL");
_Static_assert(SIGFPE != SIGINT, "SIGFPE != SIGINT");
_Static_assert(SIGFPE != SIGSEGV, "SIGFPE != SIGSEGV");
_Static_assert(SIGFPE != SIGTERM, "SIGFPE != SIGTERM");
_Static_assert(SIGILL != SIGINT, "SIGILL != SIGINT");
_Static_assert(SIGILL != SIGSEGV, "SIGILL != SIGSEGV");
_Static_assert(SIGILL != SIGTERM, "SIGILL != SIGTERM");
_Static_assert(SIGINT != SIGSEGV, "SIGINT != SIGSEGV");
_Static_assert(SIGINT != SIGTERM, "SIGINT != SIGTERM");
_Static_assert(SIGSEGV != SIGTERM, "SIGSEGV != SIGTERM");
