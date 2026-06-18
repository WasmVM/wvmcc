// M2-8: <errno.h>.
//
// Minimal POSIX-style error number constants. The set we ship here is the
// subset that wasmvm host modules actually return; user code that needs
// a value we haven't defined can use the integer literal directly until
// we add more.
#ifndef _WVMCC_ERRNO_H
#define _WVMCC_ERRNO_H

extern int errno;

#define EPERM           1
#define ENOENT          2
#define EIO             5
#define EBADF           9
#define ENOMEM         12
#define EACCES         13
#define EFAULT         14
#define EBUSY          16
#define EEXIST         17
#define ENODEV         19
#define ENOTDIR        20
#define EISDIR         21
#define EINVAL         22
#define ENFILE         23
#define EMFILE         24
#define ENOSPC         28
#define EROFS          30
#define EPIPE          32
#define EDOM           33
#define ERANGE         34
#define ENAMETOOLONG   36
#define ENOSYS         38
#define ENOTEMPTY      39
#define EILSEQ         84

#endif // _WVMCC_ERRNO_H
