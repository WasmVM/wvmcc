// M2-12: <stdio.h>.
//
// FILE plus core unformatted I/O. The printf family is declared here
// but defined in M2-13 (integer/string) and M2-14 (float via Ryu).
#ifndef _WVMCC_STDIO_H
#define _WVMCC_STDIO_H

#include <stddef.h>
#include <stdarg.h>

typedef struct FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

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
