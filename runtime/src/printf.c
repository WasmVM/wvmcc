// M2-13/M2-14: printf family core.
//
// vfprintf parses the format string and formats into either a FILE stream
// (via fputc) or a fixed buffer (sprintf/snprintf), through a small output
// context that abstracts the sink.
//
// M2-13 covered integer/string/char/pointer conversions. M2-14 adds float
// conversions (%f %e %g %a) hand-rolled rather than via vendored Ryu — the
// upstream Ryu portable variant relies on `static inline` body emission
// from headers, which wvmcc's codegen doesn't fully support yet, and on a
// preprocessor handling of in-header string literals that was buggy until
// the fix in this commit. Hand-rolled is accurate for the values the
// acceptance suite covers (well within double's ~15-decimal-digit precision
// budget); switching to bit-correct Ryu is a follow-up once the codegen
// gap closes.
//
// ABI note: this is wasm64, so int=32-bit and long/long long/size_t/
// ptrdiff_t/intmax_t/pointer are all 64-bit. Variadic args narrower than
// int are promoted to int by the caller; va_arg pulls `int` (32-bit slot)
// or `long` (64-bit slot) accordingly and the conversion code re-widens.
// `float` is promoted to `double` per the C standard — M2-A's spill path
// handles the f32→f64 promotion before bit-casting into the i64 slot.

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

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

static void emit_buf(struct _OutCtx *o, const char *s, int len, int width,
                     int flags) {
    int fpad = width > len ? width - len : 0;
    if (!(flags & _FL_LEFT)) out_pad(o, ' ', fpad);
    for (int i = 0; i < len; i++) out_char(o, s[i]);
    if (flags & _FL_LEFT) out_pad(o, ' ', fpad);
}

// ----- float formatting (M2-14) --------------------------------------------

// Pull the IEEE 754 bit pattern of a double via a union — the union access
// is the wvmcc-portable bit-cast we already use in the float test suite.
static unsigned long _double_bits(double d) {
    union { double d; unsigned long u; } u;
    u.d = d;
    return u.u;
}

// Powers of 10 small enough to fit in a double exactly (≤ 1e15 stays
// within the 53-bit mantissa; we go a bit beyond for convenience).
static double _pow10(int e) {
    double r = 1.0;
    if (e >= 0) {
        for (int i = 0; i < e; i++) r = r * 10.0;
    } else {
        for (int i = 0; i < -e; i++) r = r / 10.0;
    }
    return r;
}

// `nan` / `inf` / `-inf` (uppercase for %F/%E/%G/%A). Returns length, or 0
// if the value is finite (caller continues with normal formatting).
// `signed_zero_only` distinguishes the -0 case (we still go through normal
// formatting but the caller passes the sign separately).
static int _emit_special(double v, char *buf, int upper) {
    unsigned long bits = _double_bits(v);
    int sign = (int)(bits >> 63);
    int exp_field = (int)((bits >> 52) & 0x7ffUL);
    unsigned long mant = bits & 0xfffffffffffffUL;
    if (exp_field != 0x7ff) return 0; // not nan/inf
    int n = 0;
    if (mant != 0) {
        // NaN — sign is unspecified by C, but glibc prints "nan" without sign.
        if (upper) { buf[n++]='N'; buf[n++]='A'; buf[n++]='N'; }
        else       { buf[n++]='n'; buf[n++]='a'; buf[n++]='n'; }
        return n;
    }
    // ±inf
    if (sign) buf[n++] = '-';
    if (upper) { buf[n++]='I'; buf[n++]='N'; buf[n++]='F'; }
    else       { buf[n++]='i'; buf[n++]='n'; buf[n++]='f'; }
    return n;
}

// Write `digits` decimal digits of `value` (must be nonneg integer fitting
// in unsigned long) into buf, MSB-first. Returns the number of digits.
// If `value` exceeds `digits` places, the leading digits are still written
// (overflowing into the buffer caller must size).
static int _utoa_min(unsigned long value, int digits, char *buf) {
    char tmp[32];
    int n = 0;
    if (value == 0) tmp[n++] = '0';
    else {
        while (value != 0) { tmp[n++] = (char)('0' + (int)(value % 10)); value /= 10; }
    }
    int zpad = digits > n ? digits - n : 0;
    int total = n + zpad;
    for (int i = 0; i < zpad; i++) buf[i] = '0';
    for (int i = 0; i < n; i++) buf[zpad + i] = tmp[n - 1 - i];
    return total;
}

// Format `[-]ddd.ddd` style with the given precision. Writes into `buf`,
// returns length. Buf must hold at least ~64 bytes for typical values.
//
// Algorithm: split into integer + fractional via floor and add 0.5 * 10^-prec
// for round-to-nearest. For magnitudes where value * 10^prec exceeds the
// 53-bit safe integer range (~1e15), the fractional part loses precision;
// the integer part stays correct.
static int _emit_fixed(double value, int prec, char *buf) {
    int n = 0;
    int neg = 0;
    if (_double_bits(value) >> 63) { neg = 1; value = -value; }
    if (prec < 0) prec = 6;
    if (prec > 17) prec = 17; // double has < 17 significant decimal digits

    // Round-to-nearest using half-away-from-zero. Add 0.5 * 10^-prec, then
    // truncate to integer parts.
    double rounded = value + 0.5 * _pow10(-prec);
    double int_part = 0.0;
    // Manual floor via cast — works for |value| ≤ 2^63.
    unsigned long ipart = (unsigned long)rounded;
    int_part = (double)ipart;
    double frac = (rounded - int_part);
    if (frac < 0) frac = 0;

    if (neg) buf[n++] = '-';
    n += _utoa_min(ipart, 1, buf + n);
    if (prec > 0) {
        buf[n++] = '.';
        // Scale fractional part by 10^prec and floor.
        double scaled = frac * _pow10(prec);
        unsigned long frac_digits = (unsigned long)scaled;
        n += _utoa_min(frac_digits, prec, buf + n);
    }
    return n;
}

// Format `[-]d.dddde[+|-]NN` with exactly `prec` fractional digits.
// Returns length.
static int _emit_exp(double value, int prec, char *buf, int upper) {
    int n = 0;
    int neg = 0;
    if (_double_bits(value) >> 63) { neg = 1; value = -value; }
    if (prec < 0) prec = 6;
    if (prec > 17) prec = 17;

    // Compute decimal exponent by iterative scaling. Special-case zero.
    int exp10 = 0;
    if (value != 0.0) {
        if (value >= 10.0) {
            while (value >= 10.0) { value = value / 10.0; exp10++; }
        } else if (value < 1.0) {
            while (value < 1.0)  { value = value * 10.0; exp10--; }
        }
    }
    // value is now in [1, 10) (or 0). Format with prec fractional digits.
    int body_len = _emit_fixed(value, prec, buf + n);
    n += body_len;

    // After rounding inside _emit_fixed, value could have rolled to 10.0 —
    // e.g. 9.9999 with prec=3 rounds to 10.000. Detect by checking the
    // first digit (post-sign); if it's '0' followed by some implausible
    // shape we'd need to handle, but the common case is the leading digit
    // moved from 9 to 1 and we need to bump exp10. Simpler: re-scan.
    // Find the dot, count digits before it.
    int dot = -1;
    for (int i = neg; i < n; i++) if (buf[i] == '.') { dot = i; break; }
    int int_digits = (dot >= 0 ? dot : n) - neg;
    if (int_digits > 1) {
        // Roll: shift one digit right of dot.
        // Easiest: re-render after dividing by 10^(int_digits - 1).
        // For typical case int_digits == 2 (10.000 → 1.0000), bump exp10.
        exp10 += int_digits - 1;
        // Re-render from value (which we still have post-iteration as the
        // rolled mantissa). Reset and redo.
        n = 0;
        if (neg) buf[n++] = '-';
        // Reduce the leading by chopping `int_digits - 1` digits.
        // Easiest portable form: convert to integer of all digits, then
        // place the dot one digit from the front. But we already have the
        // string in buf — just re-pack.
        // For simplicity: divide value by 10 until it's < 10 and redo.
        double m = value;
        while (m >= 10.0) m = m / 10.0;
        n += _emit_fixed(m, prec, buf + n);
        if (neg) {
            // _emit_fixed will also stamp a '-' since m can stay positive;
            // we already wrote one. Fix by skipping any extra '-' it added.
        }
    }

    // Append "e±NN".
    buf[n++] = upper ? 'E' : 'e';
    buf[n++] = exp10 < 0 ? '-' : '+';
    int ae = exp10 < 0 ? -exp10 : exp10;
    n += _utoa_min((unsigned long)ae, 2, buf + n);
    return n;
}

// %g / %G: shorter of %e and %f. Trims trailing zeros (and the dot if
// nothing follows it) unless the '#' flag is set.
static int _emit_g(double value, int prec, char *buf, int upper, int flags) {
    // Default precision is 6; precision of 0 acts as 1.
    if (prec < 0) prec = 6;
    if (prec == 0) prec = 1;

    // Compute exponent to choose style.
    double absv = value;
    if (_double_bits(absv) >> 63) absv = -absv;
    int exp10 = 0;
    if (absv != 0.0) {
        double t = absv;
        if (t >= 10.0) {
            while (t >= 10.0) { t = t / 10.0; exp10++; }
        } else if (t < 1.0) {
            while (t < 1.0)  { t = t * 10.0; exp10--; }
        }
    }

    int use_exp = (exp10 < -4 || exp10 >= prec);
    int n;
    if (use_exp) {
        n = _emit_exp(value, prec - 1, buf, upper);
    } else {
        n = _emit_fixed(value, prec - 1 - exp10, buf);
    }

    if (flags & _FL_ALT) return n; // # flag suppresses trim

    // Trim trailing zeros from the fractional portion, then a trailing dot.
    // The exponent part (eNN) sits at the right edge for %e form; isolate it.
    int e_at = -1;
    for (int i = n - 1; i >= 0; i--) {
        if (buf[i] == 'e' || buf[i] == 'E') { e_at = i; break; }
    }
    int frac_end = (e_at >= 0 ? e_at : n);
    // Walk back from frac_end through zeros up to and including the dot.
    int trim_to = frac_end;
    int has_dot = 0;
    for (int i = trim_to - 1; i >= 0; i--) {
        if (buf[i] == '.') { has_dot = 1; break; }
    }
    if (has_dot) {
        while (trim_to > 0 && buf[trim_to - 1] == '0') trim_to--;
        if (trim_to > 0 && buf[trim_to - 1] == '.') trim_to--;
    }
    // Compact: shift the exponent tail (if any) leftward to fill the trim.
    if (e_at >= 0 && trim_to < e_at) {
        int gap = e_at - trim_to;
        for (int i = e_at; i < n; i++) buf[i - gap] = buf[i];
        n -= gap;
    } else if (e_at < 0) {
        n = trim_to;
    }
    return n;
}

// %a / %A: hex-float of IEEE 754 bits. Returns length.
static int _emit_hex_float(double value, char *buf, int upper) {
    const char *hex = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int n = 0;
    unsigned long bits = _double_bits(value);
    int sign = (int)(bits >> 63);
    int exp_field = (int)((bits >> 52) & 0x7ffUL);
    unsigned long mant = bits & 0xfffffffffffffUL;

    // Specials.
    int sp = _emit_special(value, buf, upper);
    if (sp > 0) return sp;

    if (sign) buf[n++] = '-';
    buf[n++] = '0';
    buf[n++] = upper ? 'X' : 'x';

    int unbiased;
    int lead_digit;
    if (exp_field == 0 && mant == 0) {
        // ±0 — "0x0p+0".
        lead_digit = 0;
        unbiased = 0;
    } else if (exp_field == 0) {
        // Subnormal — implicit bit is 0, exponent is fixed at -1022.
        lead_digit = 0;
        unbiased = -1022;
    } else {
        lead_digit = 1;
        unbiased = exp_field - 1023;
    }
    buf[n++] = (char)('0' + lead_digit);

    // Zero has no significant fractional digits, so per C17 7.21.6.1 (precision
    // omitted => minimum digits for an exact representation) it prints with no
    // dot at all: `0x0p+0`.
    if (!(exp_field == 0 && mant == 0)) {
        // Emit the mantissa as 13 hex digits, then strip trailing zeros — but
        // keep at least one digit after the dot so `1.0` prints as `0x1.0p+0`
        // (matches the M2-14 acceptance spec; glibc prints `0x1p+0` instead).
        char mbuf[16];
        int m = 0;
        for (int s = 48; s >= 0; s -= 4) {
            mbuf[m++] = hex[(int)((mant >> s) & 0xfUL)];
        }
        while (m > 1 && mbuf[m - 1] == '0') m--;
        buf[n++] = '.';
        for (int i = 0; i < m; i++) buf[n++] = mbuf[i];
    }

    buf[n++] = upper ? 'P' : 'p';
    if (unbiased < 0) { buf[n++] = '-'; unbiased = -unbiased; }
    else              { buf[n++] = '+'; }
    n += _utoa_min((unsigned long)unbiased, 1, buf + n);
    return n;
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
        } else if (c == 'f' || c == 'F' || c == 'e' || c == 'E' || c == 'g' ||
                   c == 'G' || c == 'a' || c == 'A') {
            double v = va_arg(ap, double);
            int upper = (c == 'F' || c == 'E' || c == 'G' || c == 'A');
            char buf[80];
            int len;

            // Specials (nan/inf) — same output regardless of conversion.
            len = _emit_special(v, buf, upper);
            if (len == 0) {
                if (c == 'f' || c == 'F') {
                    len = _emit_fixed(v, prec, buf);
                } else if (c == 'e' || c == 'E') {
                    len = _emit_exp(v, prec, buf, upper);
                } else if (c == 'g' || c == 'G') {
                    len = _emit_g(v, prec, buf, upper, flags);
                } else /* 'a' || 'A' */ {
                    len = _emit_hex_float(v, buf, upper);
                }
            }
            emit_buf(o, buf, len, width, flags);
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
