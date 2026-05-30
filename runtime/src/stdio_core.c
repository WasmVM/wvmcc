// M2-12: <stdio.h> core implementation.
//
// FILE = fd + flags + (lazy) malloc'd buffers. Three statics back
// stdin/stdout/stderr; they receive their buffers on first I/O so we
// avoid wvmcc's current limitation around static struct initializers
// that reference array addresses.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define _F_EOF      1
#define _F_ERR      2
#define _F_READ     4
#define _F_WRITE    8
#define _F_LINEBUF  16
#define _F_UNBUF    32
#define _F_INITED   64

struct FILE {
    int fd;
    int flags;
    char *wbuf;
    int wbuf_size;
    int wbuf_pos;
    char *rbuf;
    int rbuf_size;
    int rbuf_pos;
    int rbuf_end;
};

static FILE __stdin  = { 0, _F_READ };
static FILE __stdout = { 1, _F_WRITE | _F_LINEBUF };
static FILE __stderr = { 2, _F_WRITE | _F_UNBUF };

FILE *stdin  = &__stdin;
FILE *stdout = &__stdout;
FILE *stderr = &__stderr;

static void lazy_init(FILE *f) {
    if (f->flags & _F_INITED) return;
    f->flags |= _F_INITED;
    if ((f->flags & _F_UNBUF) == 0) {
        if (f->flags & _F_WRITE) {
            f->wbuf = (char *)malloc(BUFSIZ);
            f->wbuf_size = f->wbuf ? BUFSIZ : 0;
        }
        if (f->flags & _F_READ) {
            f->rbuf = (char *)malloc(BUFSIZ);
            f->rbuf_size = f->rbuf ? BUFSIZ : 0;
        }
    }
}

static int flush_write_buf(FILE *f) {
    if (f->wbuf_pos == 0) return 0;
    int n = (int)write(f->fd, f->wbuf, f->wbuf_pos);
    if (n < 0) { f->flags |= _F_ERR; return EOF; }
    f->wbuf_pos = 0;
    return 0;
}

int fflush(FILE *f) {
    if (!f) return 0;
    return flush_write_buf(f);
}

int fputc(int c, FILE *f) {
    if (!f || (f->flags & _F_WRITE) == 0) return EOF;
    lazy_init(f);
    unsigned char b = (unsigned char)c;
    if ((f->flags & _F_UNBUF) || f->wbuf_size == 0) {
        if (write(f->fd, &b, 1) < 0) { f->flags |= _F_ERR; return EOF; }
        return c;
    }
    if (f->wbuf_pos >= f->wbuf_size) {
        if (flush_write_buf(f) < 0) return EOF;
    }
    f->wbuf[f->wbuf_pos++] = (char)b;
    if ((f->flags & _F_LINEBUF) && b == '\n') {
        if (flush_write_buf(f) < 0) return EOF;
    }
    return c;
}

int putc(int c, FILE *f) { return fputc(c, f); }
int putchar(int c)       { return fputc(c, stdout); }

int fputs(const char *s, FILE *f) {
    if (!s || !f) return EOF;
    while (*s) {
        if (fputc((unsigned char)*s, f) == EOF) return EOF;
        s++;
    }
    return 0;
}

int puts(const char *s) {
    if (fputs(s, stdout) == EOF) return EOF;
    if (fputc('\n', stdout) == EOF) return EOF;
    return 0;
}

static int refill_read_buf(FILE *f) {
    lazy_init(f);
    if ((f->flags & _F_READ) == 0 || !f->rbuf) return -1;
    int n = (int)read(f->fd, f->rbuf, f->rbuf_size);
    if (n < 0) { f->flags |= _F_ERR; return -1; }
    if (n == 0) { f->flags |= _F_EOF; return 0; }
    f->rbuf_pos = 0;
    f->rbuf_end = n;
    return n;
}

int fgetc(FILE *f) {
    if (!f) return EOF;
    if (f->rbuf_pos >= f->rbuf_end) {
        if (refill_read_buf(f) <= 0) return EOF;
    }
    return (unsigned char)f->rbuf[f->rbuf_pos++];
}

int getc(FILE *f) { return fgetc(f); }
int getchar(void) { return fgetc(stdin); }

char *fgets(char *s, int n, FILE *f) {
    if (!s || n <= 0 || !f) return (char *)0;
    int i = 0;
    while (i < n - 1) {
        int c = fgetc(f);
        if (c == EOF) {
            if (i == 0) return (char *)0;
            break;
        }
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = 0;
    return s;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *f) {
    if (!ptr || !f || size == 0 || nmemb == 0) return 0;
    size_t total = size * nmemb;
    char *p = (char *)ptr;
    size_t done = 0;
    while (done < total) {
        if (f->rbuf_pos >= f->rbuf_end) {
            if (refill_read_buf(f) <= 0) break;
        }
        int avail = f->rbuf_end - f->rbuf_pos;
        size_t want = total - done;
        size_t take = (size_t)avail < want ? (size_t)avail : want;
        memcpy(p + done, f->rbuf + f->rbuf_pos, take);
        f->rbuf_pos += (int)take;
        done += take;
    }
    return done / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f) {
    if (!ptr || !f || size == 0 || nmemb == 0) return 0;
    if ((f->flags & _F_WRITE) == 0) return 0;
    lazy_init(f);
    size_t total = size * nmemb;
    const char *p = (const char *)ptr;
    if ((f->flags & _F_UNBUF) || f->wbuf_size == 0) {
        int n = (int)write(f->fd, p, total);
        if (n < 0) { f->flags |= _F_ERR; return 0; }
        return (size_t)n / size;
    }
    size_t done = 0;
    while (done < total) {
        if (f->wbuf_pos >= f->wbuf_size) {
            if (flush_write_buf(f) < 0) return done / size;
        }
        char ch = p[done];
        f->wbuf[f->wbuf_pos++] = ch;
        done++;
        if ((f->flags & _F_LINEBUF) && ch == '\n') {
            if (flush_write_buf(f) < 0) return done / size;
        }
    }
    return nmemb;
}

int fseek(FILE *f, long offset, int whence) {
    if (!f) return -1;
    flush_write_buf(f);
    f->rbuf_pos = 0;
    f->rbuf_end = 0;
    f->flags &= ~_F_EOF;
    long r = lseek(f->fd, offset, whence);
    if (r < 0) return -1;
    return 0;
}

long ftell(FILE *f) {
    if (!f) return -1;
    long r = lseek(f->fd, 0, SEEK_CUR);
    if (r < 0) return -1;
    return r - (long)(f->rbuf_end - f->rbuf_pos) + (long)f->wbuf_pos;
}

void rewind(FILE *f) {
    if (!f) return;
    fseek(f, 0, SEEK_SET);
    f->flags &= ~_F_ERR;
}

int feof(FILE *f)    { return f ? (f->flags & _F_EOF)  : 0; }
int ferror(FILE *f)  { return f ? (f->flags & _F_ERR)  : 0; }
void clearerr(FILE *f) { if (f) f->flags &= ~(_F_EOF | _F_ERR); }

static int parse_mode(const char *m, int *o_flags, int *f_flags) {
    if (!m || !*m) return -1;
    int oflag, fflag = 0;
    if      (*m == 'r') { oflag = O_RDONLY; fflag = _F_READ; }
    else if (*m == 'w') { oflag = O_WRONLY | O_CREAT | O_TRUNC; fflag = _F_WRITE; }
    else if (*m == 'a') { oflag = O_WRONLY | O_CREAT | O_APPEND; fflag = _F_WRITE; }
    else return -1;
    m++;
    while (*m) {
        if (*m == '+') {
            oflag = (oflag & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
            fflag |= _F_READ | _F_WRITE;
        }
        m++;
    }
    *o_flags = oflag;
    *f_flags = fflag;
    return 0;
}

FILE *fopen(const char *path, const char *mode) {
    int oflag, fflag;
    if (parse_mode(mode, &oflag, &fflag) < 0) {
        errno = EINVAL;
        return (FILE *)0;
    }
    int fd = open(path, oflag, 0644);
    if (fd < 0) return (FILE *)0;
    FILE *f = (FILE *)malloc(sizeof(FILE));
    if (!f) { close(fd); return (FILE *)0; }
    f->fd = fd;
    f->flags = fflag;
    f->wbuf = (char *)0;
    f->wbuf_size = 0;
    f->wbuf_pos = 0;
    f->rbuf = (char *)0;
    f->rbuf_size = 0;
    f->rbuf_pos = 0;
    f->rbuf_end = 0;
    return f;
}

int fclose(FILE *f) {
    if (!f) return EOF;
    int r = 0;
    if (flush_write_buf(f) < 0) r = EOF;
    if (close(f->fd) < 0) r = EOF;
    free(f->wbuf);
    free(f->rbuf);
    free(f);
    return r;
}

static void itoa10(int n, char *buf) {
    char tmp[16];
    int i = 0;
    int neg = 0;
    unsigned int u;
    if (n < 0) { neg = 1; u = (unsigned int)(-n); }
    else       { u = (unsigned int)n; }
    if (u == 0) tmp[i++] = '0';
    while (u) { tmp[i++] = (char)('0' + (u % 10)); u /= 10; }
    if (neg) tmp[i++] = '-';
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = 0;
}

void perror(const char *s) {
    if (s && *s) { fputs(s, stderr); fputs(": ", stderr); }
    char buf[16];
    itoa10(errno, buf);
    fputs("errno=", stderr);
    fputs(buf, stderr);
    fputc('\n', stderr);
    fflush(stderr);
}
