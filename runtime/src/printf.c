// M2-13: printf family core — integer/string/char/pointer conversions.
//
// vfprintf parses the format string and formats into either a FILE stream
// (via fputc) or a fixed buffer (sprintf/snprintf), through a small output
// context that abstracts the sink. Float conversions (%f %e %g %a) are
// deferred to M2-14 (Ryu) — they emit a "<float?>" placeholder here.
//
// ABI note: this is wasm64, so int=32-bit and long/long long/size_t/
// ptrdiff_t/intmax_t/pointer are all 64-bit. Variadic args narrower than
// int are promoted to int by the caller; va_arg pulls `int` (32-bit slot)
// or `long` (64-bit slot) accordingly and the conversion code re-widens.

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

// ----- output context ------------------------------------------------------

struct _OutCtx {
    FILE *stream;   // non-null: write to this stream
    char *buf;      // non-null: write to this buffer
    size_t cap;     // buffer capacity incl. NUL slot (0 = unbounded, for sprintf)
    size_t count;   // total chars that *would* be written (the return value)
};

// Emit one byte to the sink. For buffers, honor the capacity (leaving room
// for the terminating NUL) but keep counting so snprintf can report the full
// length, matching C99 semantics.
static void out_char(struct _OutCtx *o, char c) {
    if (o->stream) {
        fputc((unsigned char)c, o->stream);
    } else if (o->buf) {
        if (o->cap == 0 || o->count + 1 < o->cap) {
            o->buf[o->count] = c;
        }
    }
    o->count++;
}

static void out_pad(struct _OutCtx *o, char c, int n) {
    while (n > 0) { out_char(o, c); n--; }
}

// ----- conversion flags -----------------------------------------------------

#define _FL_LEFT   1   // '-'
#define _FL_ZERO   2   // '0'
#define _FL_PLUS   4   // '+'
#define _FL_SPACE  8   // ' '
#define _FL_ALT    16  // '#'

// Length modifiers — only the "is it 64-bit" distinction matters on wasm64,
// but track explicitly for clarity.
#define _LEN_INT   0
#define _LEN_LONG  1   // l / ll / z / t / j

// Render an unsigned value into `tmp` (reverse order), return digit count.
// `digits` selects the alphabet (lowercase/uppercase hex).
static int utoa_base(unsigned long v, unsigned base, const char *digits,
                     char *tmp) {
    int i = 0;
    if (v == 0) { tmp[i++] = '0'; return i; }
    while (v != 0) {
        tmp[i++] = digits[v % base];
        v /= base;
    }
    return i;
}

// Emit an integer conversion. `uval` is the magnitude; `neg` is set for a
// negative signed value. `prefix` is an optional radix prefix ("0x"/"0X"/"0").
static void emit_int(struct _OutCtx *o, unsigned long uval, int neg,
                     int base, const char *digits, const char *prefix,
                     int width, int prec, int flags) {
    char tmp[24];
    int ndigits = utoa_base(uval, (unsigned)base, digits, tmp);

    // A precision of 0 with a zero value produces no digits.
    if (prec == 0 && uval == 0) ndigits = 0;

    // Sign / space character.
    char sign = 0;
    if (neg)                  sign = '-';
    else if (flags & _FL_PLUS)  sign = '+';
    else if (flags & _FL_SPACE) sign = ' ';

    int plen = 0;
    if (prefix) { while (prefix[plen]) plen++; }

    // Precision = minimum number of digits (zero-pad to reach it).
    int zpad = 0;
    if (prec > ndigits) zpad = prec - ndigits;

    int body = ndigits + zpad + (sign ? 1 : 0) + plen;

    // Field width padding. With '0' flag (and no explicit precision), pad
    // with zeros *after* the sign/prefix; otherwise pad with spaces.
    int fpad = width > body ? width - body : 0;
    int use_zero = (flags & _FL_ZERO) && !(flags & _FL_LEFT) && prec < 0;

    if (!(flags & _FL_LEFT) && !use_zero) out_pad(o, ' ', fpad);
    if (sign) out_char(o, sign);
    for (int k = 0; k < plen; k++) out_char(o, prefix[k]);
    if (use_zero) out_pad(o, '0', fpad);
    out_pad(o, '0', zpad);
    for (int k = ndigits - 1; k >= 0; k--) out_char(o, tmp[k]);
    if (flags & _FL_LEFT) out_pad(o, ' ', fpad);
}

static void emit_str(struct _OutCtx *o, const char *s, int width, int prec,
                     int flags) {
    if (!s) s = "(null)";
    int len = 0;
    while (s[len] && (prec < 0 || len < prec)) len++;
    int fpad = width > len ? width - len : 0;
    if (!(flags & _FL_LEFT)) out_pad(o, ' ', fpad);
    for (int i = 0; i < len; i++) out_char(o, s[i]);
    if (flags & _FL_LEFT) out_pad(o, ' ', fpad);
}

// ----- core -----------------------------------------------------------------

int vfprintf(FILE *stream, const char *fmt, va_list ap);

static int do_format(struct _OutCtx *o, const char *fmt, va_list ap) {
    const char *lower = "0123456789abcdef";
    const char *upper = "0123456789ABCDEF";

    for (; *fmt; fmt++) {
        if (*fmt != '%') { out_char(o, *fmt); continue; }
        fmt++; // consume '%'
        if (*fmt == '%') { out_char(o, '%'); continue; }

        // 1. Flags.
        int flags = 0;
        for (;; fmt++) {
            if      (*fmt == '-') flags |= _FL_LEFT;
            else if (*fmt == '0') flags |= _FL_ZERO;
            else if (*fmt == '+') flags |= _FL_PLUS;
            else if (*fmt == ' ') flags |= _FL_SPACE;
            else if (*fmt == '#') flags |= _FL_ALT;
            else break;
        }

        // 2. Width (digits or '*').
        int width = 0;
        if (*fmt == '*') {
            width = va_arg(ap, int);
            if (width < 0) { flags |= _FL_LEFT; width = -width; }
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
        }

        // 3. Precision (.N or .* ; -1 means "unset").
        int prec = -1;
        if (*fmt == '.') {
            fmt++;
            prec = 0;
            if (*fmt == '*') {
                prec = va_arg(ap, int);
                if (prec < 0) prec = -1; // negative precision => unset
                fmt++;
            } else {
                while (*fmt >= '0' && *fmt <= '9') {
                    prec = prec * 10 + (*fmt - '0');
                    fmt++;
                }
            }
        }

        // 4. Length modifier.
        int lenmod = _LEN_INT;
        if (*fmt == 'h') {
            fmt++;
            if (*fmt == 'h') fmt++;          // hh — still promoted to int
        } else if (*fmt == 'l') {
            fmt++;
            lenmod = _LEN_LONG;
            if (*fmt == 'l') fmt++;          // ll
        } else if (*fmt == 'z' || *fmt == 't' || *fmt == 'j') {
            lenmod = _LEN_LONG;
            fmt++;
        }

        // 5. Conversion specifier.
        char c = *fmt;
        if (c == 'd' || c == 'i') {
            long v;
            if (lenmod == _LEN_LONG) v = va_arg(ap, long);
            else                     v = (long)va_arg(ap, int);
            int neg = v < 0;
            unsigned long uv = neg ? (unsigned long)(-v) : (unsigned long)v;
            emit_int(o, uv, neg, 10, lower, 0, width, prec, flags);
        } else if (c == 'u' || c == 'x' || c == 'X' || c == 'o') {
            unsigned long uv;
            if (lenmod == _LEN_LONG) uv = (unsigned long)va_arg(ap, long);
            else uv = (unsigned long)(unsigned)va_arg(ap, int);
            int base = 10;
            const char *digits = lower;
            const char *prefix = 0;
            if (c == 'x') { base = 16; }
            else if (c == 'X') { base = 16; digits = upper; }
            else if (c == 'o') { base = 8; }
            if ((flags & _FL_ALT) && uv != 0) {
                if (c == 'x') prefix = "0x";
                else if (c == 'X') prefix = "0X";
                else if (c == 'o') prefix = "0";
            }
            emit_int(o, uv, 0, base, digits, prefix, width, prec, flags);
        } else if (c == 'c') {
            char ch = (char)va_arg(ap, int);
            int fpad = width > 1 ? width - 1 : 0;
            if (!(flags & _FL_LEFT)) out_pad(o, ' ', fpad);
            out_char(o, ch);
            if (flags & _FL_LEFT) out_pad(o, ' ', fpad);
        } else if (c == 's') {
            const char *s = va_arg(ap, char *);
            emit_str(o, s, width, prec, flags);
        } else if (c == 'p') {
            unsigned long uv = (unsigned long)va_arg(ap, void *);
            emit_int(o, uv, 0, 16, lower, "0x", width, -1, flags);
        } else if (c == 'f' || c == 'e' || c == 'E' || c == 'g' ||
                   c == 'G' || c == 'a' || c == 'A') {
            // Deferred to M2-14 (Ryu). Consume the (double) arg and emit a
            // placeholder so the format string stays in sync with the args.
            (void)va_arg(ap, double);
            emit_str(o, "<float?>", width, -1, flags);
        } else if (c == 0) {
            break; // trailing '%' at end of string
        } else {
            // Unknown specifier: emit it verbatim (including the '%').
            out_char(o, '%');
            out_char(o, c);
        }
    }
    return (int)o->count;
}

// ----- public entry points --------------------------------------------------

int vfprintf(FILE *stream, const char *fmt, va_list ap) {
    struct _OutCtx o;
    o.stream = stream;
    o.buf = 0;
    o.cap = 0;
    o.count = 0;
    return do_format(&o, fmt, ap);
}

int vprintf(const char *fmt, va_list ap) {
    return vfprintf(stdout, fmt, ap);
}

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
    struct _OutCtx o;
    o.stream = 0;
    o.buf = buf;
    o.cap = n;
    o.count = 0;
    int r = do_format(&o, fmt, ap);
    if (buf && n > 0) {
        size_t term = o.count < n ? o.count : n - 1;
        buf[term] = 0;
    }
    return r;
}

int vsprintf(char *buf, const char *fmt, va_list ap) {
    struct _OutCtx o;
    o.stream = 0;
    o.buf = buf;
    o.cap = 0;
    o.count = 0;
    int r = do_format(&o, fmt, ap);
    if (buf) buf[o.count] = 0;
    return r;
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return r;
}

int fprintf(FILE *stream, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(stream, fmt, ap);
    va_end(ap);
    return r;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsprintf(buf, fmt, ap);
    va_end(ap);
    return r;
}

int snprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}
