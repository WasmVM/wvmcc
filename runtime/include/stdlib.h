// M2-10: <stdlib.h> core.
//
// Excludes <string.h>'s strdup-style functions; malloc family lives
// here as a forward declaration — the actual implementation comes in
// M2-11.
#ifndef _WVMCC_STDLIB_H
#define _WVMCC_STDLIB_H

#include <stddef.h>

/* Process control */
_Noreturn void exit(int status);
_Noreturn void _Exit(int status);
_Noreturn void abort(void);
int atexit(void (*func)(void));
_Noreturn void quick_exit(int status);
int at_quick_exit(void (*func)(void));

/* Conversion */
int       atoi(const char *s);
long      atol(const char *s);
long long atoll(const char *s);
double    atof(const char *s);
long      strtol(const char *s, char **endptr, int base);
unsigned long strtoul(const char *s, char **endptr, int base);
long long strtoll(const char *s, char **endptr, int base);
unsigned long long strtoull(const char *s, char **endptr, int base);
double      strtod(const char *s, char **endptr);
float       strtof(const char *s, char **endptr);
long double strtold(const char *s, char **endptr);

/* Integer arithmetic (7.22.6). div_t/ldiv_t/lldiv_t hold a quotient and
 * remainder of the corresponding signed integer type. */
typedef struct { int       quot, rem; } div_t;
typedef struct { long      quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

div_t   div(int numer, int denom);
ldiv_t  ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

/* Math */
int       abs(int n);
long      labs(long n);
long long llabs(long long n);

/* Sort/search */
void  qsort(void *base, size_t nmemb, size_t size,
            int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));

/* PRNG */
int  rand(void);
void srand(unsigned int seed);
#define RAND_MAX 0x7fffffff

/* Memory (defined in M2-11) */
void *malloc(size_t size);
void *aligned_alloc(size_t alignment, size_t size);
void  free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* Largest number of bytes in a multibyte character for the current locale
 * (7.22p3). wvmcc supports only the "C" locale, where it is 1. */
#define MB_CUR_MAX ((size_t)1)

#endif // _WVMCC_STDLIB_H
