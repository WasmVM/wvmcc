// M2-9: <unistd.h>.
//
// POSIX-ish file I/O. Wraps the `sys_fs` host module; failures map to
// the standard `errno = -ret; return -1;` pattern.
#ifndef _WVMCC_UNISTD_H
#define _WVMCC_UNISTD_H

#include <stddef.h>

#define ssize_t long
#define off_t   long

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int     close(int fd);
off_t   lseek(int fd, off_t offset, int whence);
int     unlink(const char *path);

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#endif // _WVMCC_UNISTD_H
