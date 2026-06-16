#ifndef _WVMCC_SIGNAL_H
#define _WVMCC_SIGNAL_H

/* <signal.h> (C17 7.14). wvmcc is freestanding with no real signal delivery;
 * the interface is provided for source compatibility (signal() installs a
 * handler that is never invoked, raise() reports failure). */

typedef int sig_atomic_t;

/* Handler values: distinct pointers that are never valid handler addresses. */
#define SIG_DFL ((void (*)(int))0)
#define SIG_ERR ((void (*)(int))-1)
#define SIG_IGN ((void (*)(int))1)

/* The signal numbers required by 7.14p3. */
#define SIGABRT 6
#define SIGFPE  8
#define SIGILL  4
#define SIGINT  2
#define SIGSEGV 11
#define SIGTERM 15

void (*signal(int sig, void (*func)(int)))(int);
int raise(int sig);

#endif /* _WVMCC_SIGNAL_H */
