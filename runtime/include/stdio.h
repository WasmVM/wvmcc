// M2-12: <stdio.h>.
//
// FILE plus core unformatted I/O. The printf family is declared here
// but defined in M2-13 (integer/string) and M2-14 (float via Ryu).
#ifndef _WVMCC_STDIO_H
#define _WVMCC_STDIO_H

#include <stddef.h>
#include <stdarg.h>

// FILE (C17 7.21.1p2): a complete object type recording all stream control
// information. The layout is exposed here (rather than left opaque) so it is a
// sizeable object type — `sizeof(FILE)` is valid — while the backing objects
// for stdin/stdout/stderr still live in stdio_core.c. The `_F_*` flag bits used
// in `flags` are an implementation detail private to stdio_core.c.
typedef struct FILE {
    int fd;
    int flags;
    char *wbuf;
    int wbuf_size;
    int wbuf_pos;
    char *rbuf;
    int rbuf_size;
    int rbuf_pos;
    int rbuf_end;
} FILE;

// fpos_t (C17 7.21.1p2): a complete object type able to record every position
// within a file. A byte offset (i64) suffices for wvmcc's stream model.
typedef long fpos_t;

// stdin/stdout/stderr are the standard FILE* macros (C11 7.21.1). The backing
// FILE objects live in stdio_core.c; the macros take their address in *code* —
// so the pointer is materialized at each use site (a relocatable address)
// rather than stored as a data-to-data pointer that would need data-segment
// relocation.
extern FILE __wvmcc_stdin;
extern FILE __wvmcc_stdout;
extern FILE __wvmcc_stderr;
#define stdin  (&__wvmcc_stdin)
#define stdout (&__wvmcc_stdout)
#define stderr (&__wvmcc_stderr)

FILE *fopen(const char *path, const char *mode);
int   fclose(FILE *stream);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int  fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int  fflush(FILE *stream);

int  feof(FILE *stream);
int  ferror(FILE *stream);
void clearerr(FILE *stream);

int  fputc(int c, FILE *stream);
int  fputs(const char *s, FILE *stream);
int  putc(int c, FILE *stream);
int  putchar(int c);
int  puts(const char *s);
int  fgetc(FILE *stream);
int  getc(FILE *stream);
int  getchar(void);
char *fgets(char *s, int n, FILE *stream);

void perror(const char *s);

#define EOF (-1)

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 256

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

int printf(const char *fmt, ...);
int fprintf(FILE *stream, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t n, const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int vfprintf(FILE *stream, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);

#endif // _WVMCC_STDIO_H
