/* tests/standard/libc/stddef/size_t.c — LIBC-stddef-size_t-01 (C17 7.19p2).
 * Verify=static-assert. size_t is the unsigned integer type of the result
 * of the sizeof operator. B-impl: wvmcc documents LP64 (size_t is 64-bit). */
#include <stddef.h>

/* Unsigned: conversion of -1 wraps to the (positive) maximum value. */
_Static_assert((size_t)-1 > 0, "size_t is unsigned");

/* sizeof yields a value of type size_t (7.19p2 / 6.5.3.4p5). */
_Static_assert(_Generic(sizeof(0), size_t: 1, default: 0),
               "sizeof(...) has type size_t");

/* Documented implementation choice (LP64): size_t is 64-bit. */
_Static_assert(sizeof(size_t) == 8, "LP64: sizeof(size_t) == 8");
