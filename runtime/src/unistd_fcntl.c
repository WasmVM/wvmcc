// M2-9: <unistd.h> + <fcntl.h> implementations.
//
// Thin wrappers around `sys_fs`. The host returns negative errno on
// failure; we translate that to the POSIX `errno = -ret; return -1;`
// pattern. `open` is variadic because of the optional `mode` arg when
// `O_CREAT` is set — that's the only consumer of M2-A in libc so far.

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

__attribute__((import_module("sys_fs"), import_name("open")))
int sys_fs_open(const char *path, size_t path_len, int flags, int mode);

__attribute__((import_module("sys_fs"), import_name("close")))
int sys_fs_close(int fd);

__attribute__((import_module("sys_fs"), import_name("read")))
int sys_fs_read(int fd, void *buf, size_t buf_len);

__attribute__((import_module("sys_fs"), import_name("write")))
int sys_fs_write(int fd, const void *buf, size_t buf_len);

__attribute__((import_module("sys_fs"), import_name("lseek")))
long sys_fs_lseek(int fd, long offset, int whence);

__attribute__((import_module("sys_fs"), import_name("unlink")))
int sys_fs_unlink(const char *path, size_t path_len);

ssize_t read(int fd, void *buf, size_t n) {
    int r = sys_fs_read(fd, buf, n);
    if (r < 0) { errno = -r; return -1; }
    return r;
}

ssize_t write(int fd, const void *buf, size_t n) {
    int r = sys_fs_write(fd, buf, n);
    if (r < 0) { errno = -r; return -1; }
    return r;
}

int close(int fd) {
    int r = sys_fs_close(fd);
    if (r < 0) { errno = -r; return -1; }
    return r;
}

off_t lseek(int fd, off_t offset, int whence) {
    long r = sys_fs_lseek(fd, offset, whence);
    if (r < 0) { errno = (int)(-r); return -1; }
    return r;
}

int unlink(const char *path) {
    int r = sys_fs_unlink(path, strlen(path));
    if (r < 0) { errno = -r; return -1; }
    return r;
}

int open(const char *path, int flags, ...) {
    int mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    int r = sys_fs_open(path, strlen(path), flags, mode);
    if (r < 0) { errno = -r; return -1; }
    return r;
}
