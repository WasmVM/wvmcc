// M2-9: <fcntl.h>.
#ifndef _WVMCC_FCNTL_H
#define _WVMCC_FCNTL_H

int open(const char *path, int flags, ...);

#define O_RDONLY   0x0000
#define O_WRONLY   0x0001
#define O_RDWR     0x0002
#define O_CREAT    0x0040
#define O_EXCL     0x0080
#define O_TRUNC    0x0200
#define O_APPEND   0x0400

#endif // _WVMCC_FCNTL_H
