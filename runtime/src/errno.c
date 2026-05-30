// M2-8: <errno.h>.
//
// Single-threaded model: a plain file-scope int, no TLS, no per-thread
// indirection. Real OSes use a per-thread location, but wasmvm wraps a
// single host thread per module instance so a global is fine for v1.

int errno;
