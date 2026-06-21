# Libc Standard Catalog (C17 Clause 7)

Test-case catalog for wvmcc's compliance with the C17 **library**, organized by header. Column
definitions, the ID scheme, and all value vocabularies are in [`README.md`](README.md). Expected
behavior derives from the C17 standard and `docs/spec.md` — never from another compiler.

Schema: **ID · Spec § · Entity · Kind · Test case · Category · Status · Verify · Notes**
(`Kind` ∈ `fn` / `fn-macro` / `obj-macro` / `type` / `pragma`).

> **Build progress.** Complete — all C17 library headers §§7.2–7.31. Headers are presented in
> (roughly) freestanding-first order so the compile-time-verifiable surface comes first.

> **Reality checks (from materializing the suite — `tests/standard/`).**
> - **`<float.h>`, `<iso646.h>`, `<stdalign.h>`, `<stdnoreturn.h>` are not yet present in
>   `runtime/include`** (freestanding-required per 4p6) — their rows are `deferred` until the headers
>   land, flagged per section.
> - **`sizeof`/`_Alignof`/casts are rejected in `_Static_assert`** (wvmcc ICE-evaluator gap, **#81**),
>   so type-width/alignment/signedness `static-assert` rows cannot be verified yet. Only
>   integer-constant-**macro** rows are live; affected rows note `#81`. `<stddef.h>` has no live
>   `static-assert` row as a result.
>
> **Status basis.** `supported`/`partial` reflect the `runtime/` libc; the minimal freestanding
> headers (`<limits.h> <stdarg.h> <stdbool.h> <stddef.h> <stdint.h>`, per 4p6) are present and
> testable in `-ffreestanding`. `<complex.h>`,
> `<tgmath.h>`, `<stdatomic.h>`, `<threads.h>`, `<fenv.h>`, `<uchar.h>`, `<wchar.h>`,
> `<wctype.h>`, `<locale.h>`, `<signal.h>`, `<setjmp.h>` are `deferred`/`by-design` per
> `docs/spec.md`.

---

## `<float.h>` (7.7)

Characteristics of floating types. All macros are `obj-macro` constant expressions; the integer
ones are `#if`-usable. Values follow IEEE-754 binary32/binary64 with `long double` aliased to
`double` (`docs/spec.md`). See also 5.2.4.2.2.

> ⚠ **Not yet provided by `runtime/include`** (freestanding-required, 4p6). Every row below is
> effectively **`deferred`** until `<float.h>` lands; the listed Status is the intended end state.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-float-FLT_ROUNDS-01` | 7.7p3 | FLT_ROUNDS | obj-macro | Defined; value characterizes the rounding mode | B-impl | partial | static-assert | `docs/spec.md`: round-to-nearest → 1 |
| `LIBC-float-FLT_EVAL_METHOD-01` | 7.7p3 | FLT_EVAL_METHOD | obj-macro | Defined; characterizes evaluation format | B-impl | supported | static-assert | wvmcc: 0 (evaluate to type) |
| `LIBC-float-FLT_RADIX-01` | 5.2.4.2.2p11 | FLT_RADIX | obj-macro | `== 2`, `#if`-usable | B-impl | supported | static-assert | IEEE-754 |
| `LIBC-float-FLT_MANT_DIG-01` | 5.2.4.2.2p11 | FLT_MANT_DIG / DBL_MANT_DIG / LDBL_MANT_DIG | obj-macro | Significand digit counts (24 / 53 / 53) | B-impl | supported | static-assert | LDBL == DBL (alias) |
| `LIBC-float-FLT_DIG-01` | 5.2.4.2.2p11 | FLT_DIG / DBL_DIG / LDBL_DIG | obj-macro | Decimal precision (≥ 6 / 10 / 10) | B-impl | supported | static-assert | |
| `LIBC-float-DECIMAL_DIG-01` | 5.2.4.2.2p11 | DECIMAL_DIG | obj-macro | Widest-type round-trip decimal digits (≥ 10) | B-impl | supported | static-assert | |
| `LIBC-float-FLT_MIN_EXP-01` | 5.2.4.2.2p11 | FLT_MIN_EXP / *_MIN_EXP / *_MAX_EXP | obj-macro | Min/max binary exponents per type | B-impl | supported | static-assert | |
| `LIBC-float-FLT_MIN_10_EXP-01` | 5.2.4.2.2p11 | FLT_MIN_10_EXP / *_MAX_10_EXP | obj-macro | Min/max decimal exponents (≤ -37 / ≥ +37) | B-impl | supported | static-assert | |
| `LIBC-float-FLT_MAX-01` | 5.2.4.2.2p12 | FLT_MAX / DBL_MAX / LDBL_MAX | obj-macro | Max finite value (≥ 1E37) | B-impl | supported | static-assert | |
| `LIBC-float-FLT_MIN-01` | 5.2.4.2.2p13 | FLT_MIN / DBL_MIN / LDBL_MIN | obj-macro | Min normalized value (≤ 1E-37) | B-impl | supported | static-assert | |
| `LIBC-float-FLT_EPSILON-01` | 5.2.4.2.2p13 | FLT_EPSILON / DBL_EPSILON / LDBL_EPSILON | obj-macro | Difference 1 → next representable | B-impl | supported | static-assert | |
| `LIBC-float-FLT_TRUE_MIN-01` | 5.2.4.2.2p13 | FLT_TRUE_MIN / *_TRUE_MIN | obj-macro | Min positive (subnormal) value | B-impl | partial | static-assert | |
| `LIBC-float-FLT_HAS_SUBNORM-01` | 5.2.4.2.2p10 | FLT_HAS_SUBNORM / *_HAS_SUBNORM | obj-macro | Subnormal support flag | B-impl | partial | static-assert | IEEE-754 → 1 |

## `<iso646.h>` (7.9)

Alternative spellings — eleven object-like macros for operator tokens.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-iso646-and-01` | 7.9p1 | and / and_eq / bitand / bitor / compl / not / not_eq / or / or_eq / xor / xor_eq | obj-macro | Each expands to its operator (`and`→`&&`, `bitand`→`&`, `compl`→`~`, …) | Positive | deferred | static-assert | **not yet in `runtime/include`** (freestanding-required, 4p6) |

## `<limits.h>` (7.10)

Sizes of integer types — all `obj-macro` ICEs. Values follow wvmcc's LP64 model
(`docs/spec.md`): `int` 32-bit, `long` 64-bit. See also 5.2.4.2.1.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-limits-CHAR_BIT-01` | 5.2.4.2.1 | CHAR_BIT | obj-macro | `== 8` | B-impl | supported | static-assert | |
| `LIBC-limits-SCHAR-01` | 5.2.4.2.1 | SCHAR_MIN / SCHAR_MAX / UCHAR_MAX | obj-macro | `-128 / 127 / 255` | B-impl | supported | static-assert | `docs/spec.md`: signed `char` |
| `LIBC-limits-CHAR-01` | 5.2.4.2.1p2 | CHAR_MIN / CHAR_MAX | obj-macro | Equal to `SCHAR_*` (signed `char`) | B-impl | supported | static-assert | `docs/spec.md`: signed `char` default |
| `LIBC-limits-MB_LEN_MAX-01` | 5.2.4.2.1 | MB_LEN_MAX | obj-macro | `>= 1` (wvmcc: 4) | B-impl | supported | static-assert | `runtime/include/limits.h` defines **4** (UTF-8 max bytes); standard floor is 1 |
| `LIBC-limits-SHRT-01` | 5.2.4.2.1 | SHRT_MIN / SHRT_MAX / USHRT_MAX | obj-macro | `-32768 / 32767 / 65535` | B-impl | supported | static-assert | 16-bit |
| `LIBC-limits-INT-01` | 5.2.4.2.1 | INT_MIN / INT_MAX / UINT_MAX | obj-macro | `-2147483648 / 2147483647 / 4294967295` | B-impl | supported | static-assert | 32-bit |
| `LIBC-limits-LONG-01` | 5.2.4.2.1 | LONG_MIN / LONG_MAX / ULONG_MAX | obj-macro | 64-bit bounds (LP64) | B-impl | supported | static-assert | `docs/spec.md`: `long` 64-bit |
| `LIBC-limits-LLONG-01` | 5.2.4.2.1 | LLONG_MIN / LLONG_MAX / ULLONG_MAX | obj-macro | 64-bit bounds | B-impl | supported | static-assert | |
| `LIBC-limits-ICE-01` | 5.2.4.2.1p1 | (all) | obj-macro | Every macro is usable in a `#if` directive | Positive | supported | static-assert | |

## `<stdalign.h>` (7.15)

> ⚠ **Not yet provided by `runtime/include`** (freestanding-required, 4p6); rows below are `deferred`
> until the header lands. (`_Alignas`/`_Alignof` keywords work; `_Alignof` in an ICE is additionally
> gated by #81.)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stdalign-alignas-01` | 7.15p2 | alignas | obj-macro | Expands to `_Alignas` | Positive | deferred | static-assert | header absent; runtime over-alignment partial |
| `LIBC-stdalign-alignof-01` | 7.15p2 | alignof | obj-macro | Expands to `_Alignof` | Positive | deferred | static-assert | header absent; `_Alignof` in ICE also blocked on #81 |
| `LIBC-stdalign-defined-01` | 7.15p3 | __alignas_is_defined / __alignof_is_defined | obj-macro | Each defined as `1` | Positive | deferred | static-assert | header absent |

## `<stdarg.h>` (7.16)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stdarg-va_list-01` | 7.16p3 | va_list | type | Object type for traversing variable arguments | Positive | partial | exit | variadic ABI-limited (`docs/spec.md`) |
| `LIBC-stdarg-va_start-01` | 7.16.1.4 | va_start | fn-macro | Initializes `va_list`; argument after the last named parameter | Positive | partial | exit | |
| `LIBC-stdarg-va_arg-01` | 7.16.1.1 | va_arg | fn-macro | Yields the next argument with the given type (post-default-promotion) | Positive | partial | exit | |
| `LIBC-stdarg-va_copy-01` | 7.16.1.2 | va_copy | fn-macro | Copies a `va_list` state | Positive | partial | exit | |
| `LIBC-stdarg-va_end-01` | 7.16.1.3 | va_end | fn-macro | Ends traversal | Positive | partial | exit | |
| `LIBC-stdarg-va_arg-02` | 7.16.1.1p2 | va_arg | fn-macro | A type mismatch or reading past the last argument is undefined | B-undef | partial | none | documentation |

## `<stdbool.h>` (7.18)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stdbool-bool-01` | 7.18p1 | bool | obj-macro | Expands to `_Bool` | Positive | supported | static-assert | freestanding-required; `sizeof(bool)`/cast-normalization checks **blocked on #81** |
| `LIBC-stdbool-true-01` | 7.18p1 | true / false | obj-macro | Expand to `1` / `0` | Positive | supported | static-assert | |
| `LIBC-stdbool-defined-01` | 7.18p1 | __bool_true_false_are_defined | obj-macro | Defined as `1` | Positive | supported | static-assert | |

## `<stddef.h>` (7.19)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stddef-size_t-01` | 7.19p2 | size_t | type | Unsigned `sizeof` result type | B-impl | supported | static-assert | LP64: 64-bit. `#define` (typedef-gap workaround). Size check via `sizeof` **blocked on #81** |
| `LIBC-stddef-ptrdiff_t-01` | 7.19p2 | ptrdiff_t | type | Signed pointer-difference type | B-impl | supported | static-assert | LP64: 64-bit signed. Size check **blocked on #81** |
| `LIBC-stddef-max_align_t-01` | 7.19p2 | max_align_t | type | Type with the greatest fundamental alignment | B-impl | deferred | static-assert | **not yet in `runtime/include/stddef.h`**; intended `_Alignof == 8` |
| `LIBC-stddef-wchar_t-01` | 7.19p2 | wchar_t | type | Wide-character type | B-impl | partial | static-assert | wvmcc placeholder `int` (`#define`). Size check **blocked on #81** |
| `LIBC-stddef-NULL-01` | 7.19p3 | NULL | obj-macro | A null pointer constant | Positive | supported | exit | `(void*)0`; verified at runtime (pointer comparison is not an ICE) |
| `LIBC-stddef-offsetof-01` | 7.19p3 | offsetof | fn-macro | Byte offset of a member | Positive | supported | exit | wvmcc's `offsetof` is the null-pointer trick → address constant, **not an ICE**; verify at runtime, not via `_Static_assert` |

## `<stdint.h>` (7.20)

Exact/least/fast-width integer types, their limit macros, and constant-builder macros. Values per
LP64 (`docs/spec.md`).

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stdint-intN_t-01` | 7.20.1.1 | int8_t / int16_t / int32_t / int64_t (+ unsigned) | type | Exact-width two's-complement types; `sizeof` is exactly N/8 | Positive | supported | static-assert | width via `sizeof` **blocked on #81** |
| `LIBC-stdint-int_leastN_t-01` | 7.20.1.2 | int_least8_t … int_least64_t (+ unsigned) | type | Minimum-width types; `sizeof` ≥ N/8 | Positive | supported | static-assert | width via `sizeof` **blocked on #81** |
| `LIBC-stdint-int_fastN_t-01` | 7.20.1.3 | int_fast8_t … int_fast64_t (+ unsigned) | type | Fastest minimum-width types | B-impl | supported | static-assert | impl chooses underlying type; width via `sizeof` **blocked on #81** |
| `LIBC-stdint-intptr_t-01` | 7.20.1.4 | intptr_t / uintptr_t | type | Integer types that round-trip an object pointer | B-impl | supported | static-assert | LP64: 64-bit; width via `sizeof` **blocked on #81** |
| `LIBC-stdint-intmax_t-01` | 7.20.1.5 | intmax_t / uintmax_t | type | Greatest-width integer types | B-impl | supported | static-assert | 64-bit; width via `sizeof` **blocked on #81** |
| `LIBC-stdint-INTN_limits-01` | 7.20.2.1 | INT8_MIN … INT64_MAX / UINTN_MAX | obj-macro | Exact-width limit macros; `#if`-usable | Positive | supported | static-assert | |
| `LIBC-stdint-INT_LEAST/FAST-01` | 7.20.2.2,7.20.2.3 | INT_LEASTN_* / INT_FASTN_* limit macros | obj-macro | Least/fast-width limit macros | Positive | supported | static-assert | |
| `LIBC-stdint-other-limits-01` | 7.20.2.4,7.20.2.5 | INTPTR_* / INTMAX_* / PTRDIFF_* / SIZE_MAX / SIG_ATOMIC_* / WCHAR_* / WINT_* | obj-macro | Other-type limit macros (`SIZE_MAX`, `PTRDIFF_MAX`, …) | B-impl | partial | static-assert | LP64 values |
| `LIBC-stdint-INTN_C-01` | 7.20.4 | INT8_C … INTMAX_C / UINTN_C | fn-macro | Integer-constant builder macros yield the right type/value | Positive | supported | static-assert | value landed; type/width via `sizeof` **blocked on #81** |

## `<stdnoreturn.h>` (7.23)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stdnoreturn-noreturn-01` | 7.23p1 | noreturn | obj-macro | Expands to `_Noreturn`; a `noreturn` function does not return | Positive | deferred | exit | **`<stdnoreturn.h>` not yet in `runtime/include`** (the `_Noreturn` keyword itself works, emitting trailing `unreachable`); freestanding-required (4p6) |

## `<assert.h>` (7.2)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-assert-NDEBUG-01` | 7.2p1 | NDEBUG | obj-macro | With `NDEBUG` defined before inclusion, `assert` expands to a no-op | Positive | supported | exit | |
| `LIBC-assert-assert-01` | 7.2.1.1p2 | assert | fn-macro | A nonzero argument takes no action and yields a `void` expression | Positive | supported | exit | |
| `LIBC-assert-assert-02` | 7.2.1.1p2 | assert | fn-macro | A zero argument writes diagnostic info to stderr and calls `abort` (nonzero termination) | Positive | supported | exit | test expects nonzero exit |
| `LIBC-assert-assert-03` | 7.2.1.1p2 | assert | fn-macro | The diagnostic includes the expression text, `__FILE__`, `__LINE__`, `__func__` | B-impl | partial | stdout | |
| `LIBC-assert-static_assert-01` | 7.2p3 | static_assert | obj-macro | Expands to `_Static_assert` | Positive | supported | static-assert | |

## `<errno.h>` (7.5)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-errno-errno-01` | 7.5p2 | errno | obj-macro | A modifiable `int` lvalue, settable and readable | Positive | supported | exit | cross-TU via imported address-global (`reference_extern_data_globals`) |
| `LIBC-errno-EDOM-01` | 7.5p2 | EDOM / EILSEQ / ERANGE | obj-macro | Distinct positive integer constant macros | Positive | supported | static-assert | |
| `LIBC-errno-set-01` | 7.5p3 | errno | obj-macro | A library error sets `errno` (e.g. `strtol` → `ERANGE`); it is not cleared on success | Positive | supported | exit | |

## `<ctype.h>` (7.4)

Classification/mapping functions over the "C" locale; the argument must be representable as
`unsigned char` or equal `EOF`.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-ctype-isalnum-01` | 7.4.1.1 | isalnum | fn | True iff letter or digit | Positive | supported | exit | |
| `LIBC-ctype-isalpha-01` | 7.4.1.2 | isalpha | fn | True iff a letter | Positive | supported | exit | |
| `LIBC-ctype-isblank-01` | 7.4.1.3 | isblank | fn | True for space and `\t` | Positive | partial | exit | entity undeclared — not yet implemented |
| `LIBC-ctype-iscntrl-01` | 7.4.1.4 | iscntrl | fn | True for control characters | Positive | supported | exit | |
| `LIBC-ctype-isdigit-01` | 7.4.1.5 | isdigit | fn | True iff `'0'`–`'9'` | Positive | supported | exit | |
| `LIBC-ctype-isgraph-01` | 7.4.1.6 | isgraph | fn | Printable except space | Positive | supported | exit | |
| `LIBC-ctype-islower-01` | 7.4.1.7 | islower | fn | True iff `'a'`–`'z'` | Positive | supported | exit | |
| `LIBC-ctype-isprint-01` | 7.4.1.8 | isprint | fn | Printable including space | Positive | supported | exit | |
| `LIBC-ctype-ispunct-01` | 7.4.1.9 | ispunct | fn | True for punctuation | Positive | supported | exit | |
| `LIBC-ctype-isspace-01` | 7.4.1.10 | isspace | fn | True for `' ' \t \n \v \f \r` | Positive | supported | exit | |
| `LIBC-ctype-isupper-01` | 7.4.1.11 | isupper | fn | True iff `'A'`–`'Z'` | Positive | supported | exit | |
| `LIBC-ctype-isxdigit-01` | 7.4.1.12 | isxdigit | fn | True for hexadecimal digits | Positive | supported | exit | |
| `LIBC-ctype-tolower-01` | 7.4.2.1 | tolower | fn | Uppercase → lowercase, else unchanged | Positive | supported | exit | |
| `LIBC-ctype-toupper-01` | 7.4.2.2 | toupper | fn | Lowercase → uppercase, else unchanged | Positive | supported | exit | |
| `LIBC-ctype-domain-01` | 7.4p1 | (all) | fn | An argument not representable as `unsigned char` or `EOF` is undefined | B-undef | supported | none | documentation |
| `LIBC-ctype-locale-01` | 7.4p3 | (all) | fn | Classifications beyond the "C" locale are locale-specific | B-impl | by-design | none | "C" locale only |

## `<string.h>` (7.24)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-string-memcpy-01` | 7.24.2.1 | memcpy | fn | Copies `n` bytes; returns the destination | Positive | supported | exit | |
| `LIBC-string-memcpy-02` | 7.24.2.1p2 | memcpy | fn | Overlapping copy is undefined (use `memmove`) | B-undef | supported | none | documentation |
| `LIBC-string-memmove-01` | 7.24.2.2 | memmove | fn | Copies correctly even when regions overlap | Positive | supported | exit | |
| `LIBC-string-strcpy-01` | 7.24.2.3 | strcpy | fn | Copies a string including its terminator; returns destination | Positive | supported | exit | |
| `LIBC-string-strncpy-01` | 7.24.2.4 | strncpy | fn | Copies ≤ `n` chars, null-padding the remainder | Positive | supported | exit | |
| `LIBC-string-strcat-01` | 7.24.3.1 | strcat | fn | Appends `src` to `dst` (terminated) | Positive | supported | exit | |
| `LIBC-string-strncat-01` | 7.24.3.2 | strncat | fn | Appends ≤ `n` chars then a terminator | Positive | supported | exit | |
| `LIBC-string-memcmp-01` | 7.24.4.1 | memcmp | fn | Compares `n` bytes; sign of the first differing byte pair | Positive | supported | exit | |
| `LIBC-string-strcmp-01` | 7.24.4.2 | strcmp | fn | Lexicographic string comparison | Positive | supported | exit | |
| `LIBC-string-strncmp-01` | 7.24.4.4 | strncmp | fn | Compares ≤ `n` chars | Positive | supported | exit | |
| `LIBC-string-strcoll-01` | 7.24.4.3 | strcoll | fn | Locale-collated comparison (== `strcmp` in "C") | Positive | partial | exit | "C" locale only |
| `LIBC-string-strxfrm-01` | 7.24.4.5 | strxfrm | fn | Locale transform such that `strcmp` of results matches `strcoll` | Positive | partial | exit | "C" locale only |
| `LIBC-string-memchr-01` | 7.24.5.1 | memchr | fn | Finds a byte within `n` bytes | Positive | supported | exit | |
| `LIBC-string-strchr-01` | 7.24.5.2 | strchr | fn | Finds the first occurrence of a char (incl. terminator) | Positive | supported | exit | |
| `LIBC-string-strcspn-01` | 7.24.5.3 | strcspn | fn | Length of the initial span excluding a set | Positive | supported | exit | |
| `LIBC-string-strpbrk-01` | 7.24.5.4 | strpbrk | fn | First char that is in a set | Positive | supported | exit | |
| `LIBC-string-strrchr-01` | 7.24.5.5 | strrchr | fn | Last occurrence of a char | Positive | supported | exit | |
| `LIBC-string-strspn-01` | 7.24.5.6 | strspn | fn | Length of the initial span within a set | Positive | supported | exit | |
| `LIBC-string-strstr-01` | 7.24.5.7 | strstr | fn | Finds a substring | Positive | supported | exit | |
| `LIBC-string-strtok-01` | 7.24.5.8 | strtok | fn | Tokenizes using a separator set (stateful) | Positive | supported | exit | |
| `LIBC-string-memset-01` | 7.24.6.1 | memset | fn | Fills `n` bytes with a value; returns destination | Positive | supported | exit | |
| `LIBC-string-strerror-01` | 7.24.6.2 | strerror | fn | Maps an error number to a message string | Positive | supported | exit | |
| `LIBC-string-strlen-01` | 7.24.6.3 | strlen | fn | Length excluding the terminator | Positive | supported | exit | |

## `<stdlib.h>` (7.22)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stdlib-EXIT-01` | 7.22p3 | EXIT_SUCCESS / EXIT_FAILURE / NULL / RAND_MAX / MB_CUR_MAX | obj-macro | Defined with the required forms/values | Positive | supported | static-assert | `MB_CUR_MAX == 1` |
| `LIBC-stdlib-div_t-01` | 7.22p3 | div_t / ldiv_t / lldiv_t / size_t / wchar_t | type | Result/utility types defined | Positive | supported | static-assert | |
| `LIBC-stdlib-atof-01` | 7.22.1.1 | atof | fn | Parses a `double` | Positive | supported | exit | |
| `LIBC-stdlib-atoi-01` | 7.22.1.2 | atoi / atol / atoll | fn | Parse `int`/`long`/`long long` | Positive | supported | exit | |
| `LIBC-stdlib-strtod-01` | 7.22.1.3 | strtod / strtof / strtold | fn | Parse floating with end pointer; over/underflow → `ERANGE` | Positive | supported | exit | |
| `LIBC-stdlib-strtol-01` | 7.22.1.4 | strtol / strtoll / strtoul / strtoull | fn | Parse integer with base & end pointer; range → `ERANGE` | Positive | partial | exit | entity undeclared — not yet implemented |
| `LIBC-stdlib-rand-01` | 7.22.2.1 | rand / srand | fn | Pseudo-random in `[0, RAND_MAX]`; reproducible per seed | Positive | supported | exit | |
| `LIBC-stdlib-malloc-01` | 7.22.3.4 | malloc | fn | Allocates uninitialized, suitably aligned storage | Positive | supported | exit | |
| `LIBC-stdlib-calloc-01` | 7.22.3.2 | calloc | fn | Allocates zero-initialized array storage | Positive | supported | exit | |
| `LIBC-stdlib-realloc-01` | 7.22.3.5 | realloc | fn | Resizes, preserving contents up to the lesser size | Positive | supported | exit | |
| `LIBC-stdlib-free-01` | 7.22.3.3 | free | fn | Deallocates; `free(NULL)` is a no-op | Positive | supported | exit | |
| `LIBC-stdlib-aligned_alloc-01` | 7.22.3.1 | aligned_alloc | fn | Allocates with a specified alignment | Positive | supported | exit | bump allocator advances offset to the requested alignment |
| `LIBC-stdlib-malloc-02` | 7.22.3p1 | malloc/realloc | fn | Using a pointer after `free`/`realloc` is undefined | B-undef | supported | none | documentation |
| `LIBC-stdlib-abort-01` | 7.22.4.1 | abort | fn | Abnormal termination (nonzero status), no atexit/flush | Positive | supported | exit | |
| `LIBC-stdlib-atexit-01` | 7.22.4.2 | atexit | fn | Registers handlers run in reverse order at normal exit | Positive | supported | stdout | |
| `LIBC-stdlib-exit-01` | 7.22.4.4 | exit | fn | Runs atexit handlers, flushes/closes streams, terminates with status | Positive | supported | exit | |
| `LIBC-stdlib-_Exit-01` | 7.22.4.5 | _Exit | fn | Terminates without atexit handlers or flushing | Positive | supported | exit | |
| `LIBC-stdlib-quick_exit-01` | 7.22.4.3,7.22.4.7 | quick_exit / at_quick_exit | fn | Quick-exit handler registration and termination | Positive | supported | exit | |
| `LIBC-stdlib-getenv-01` | 7.22.4.6 | getenv | fn | Looks up an environment variable | Positive | partial | exit | via `sys_proc.getenv` |
| `LIBC-stdlib-system-01` | 7.22.4.8 | system | fn | Executes a command via the host | Positive | by-design | none | no host process on WasmVM |
| `LIBC-stdlib-bsearch-01` | 7.22.5.1 | bsearch | fn | Binary search of a sorted array | Positive | supported | exit | |
| `LIBC-stdlib-qsort-01` | 7.22.5.2 | qsort | fn | Sorts via a comparator | Positive | supported | exit | |
| `LIBC-stdlib-abs-01` | 7.22.6.1 | abs / labs / llabs | fn | Integer absolute value | Positive | supported | exit | |
| `LIBC-stdlib-div-01` | 7.22.6.2 | div / ldiv / lldiv | fn | Quotient and remainder together | Positive | supported | exit | |
| `LIBC-stdlib-mb-01` | 7.22.7,7.22.8 | mblen / mbtowc / wctomb / mbstowcs / wcstombs | fn | Multibyte/wide conversions | Positive | partial | exit | "C" locale, UTF-8 |

## `<stdio.h>` (7.21)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stdio-FILE-01` | 7.21.1 | FILE / fpos_t / size_t | type | Stream/position/size types defined | Positive | supported | static-assert | |
| `LIBC-stdio-macros-01` | 7.21.1 | EOF / NULL / BUFSIZ / FOPEN_MAX / FILENAME_MAX / _IOFBF / _IOLBF / _IONBF / SEEK_SET / SEEK_CUR / SEEK_END / TMP_MAX / L_tmpnam | obj-macro | Defined with the required forms | Positive | partial | static-assert | |
| `LIBC-stdio-streams-01` | 7.21.1p3 | stdin / stdout / stderr | obj-macro | Predefined streams (fd 0/1/2) | Positive | supported | stdout | |
| `LIBC-stdio-printf-01` | 7.21.6.3 | printf | fn | Formatted output to stdout; returns the count written | Positive | supported | stdout | |
| `LIBC-stdio-fprintf-01` | 7.21.6.1 | fprintf | fn | Formatted output to a stream | Positive | supported | stdout | |
| `LIBC-stdio-sprintf-01` | 7.21.6.6 | sprintf | fn | Formatted output to a buffer | Positive | supported | exit | |
| `LIBC-stdio-snprintf-01` | 7.21.6.5 | snprintf | fn | Bounded formatted output; returns the would-be length | Positive | supported | exit | |
| `LIBC-stdio-printf-conv-01` | 7.21.6.1 | printf (conversions) | fn | `%d %i %u %o %x %X %c %s %p %%` produce correct output | Positive | supported | stdout | |
| `LIBC-stdio-printf-float-01` | 7.21.6.1 | printf (`%f %e %g %a`) | fn | Floating conversions, incl. `nan`/`inf` | Positive | supported | stdout | hand-rolled float formatting |
| `LIBC-stdio-printf-flags-01` | 7.21.6.1 | printf (flags/width/precision/length) | fn | `-+ 0#`, width, precision, `l`/`ll`/`h`/`z` modifiers | Positive | partial | stdout | |
| `LIBC-stdio-vprintf-01` | 7.21.6.8–7.21.6.14 | vprintf / vfprintf / vsprintf / vsnprintf | fn | `va_list` formatted-output variants | Positive | supported | stdout | |
| `LIBC-stdio-scanf-01` | 7.21.6.2,7.21.6.4,7.21.6.7 | scanf / fscanf / sscanf | fn | Formatted input | Positive | deferred | none | scanf family not implemented |
| `LIBC-stdio-fopen-01` | 7.21.5.3 | fopen | fn | Opens a file in a given mode | Positive | partial | exit | via `sys_fs.open` |
| `LIBC-stdio-fclose-01` | 7.21.5.1 | fclose | fn | Flushes and closes a stream | Positive | partial | exit | entity undeclared — not yet implemented |
| `LIBC-stdio-freopen-01` | 7.21.5.4 | freopen | fn | Reassociates a stream with a new file | Positive | partial | exit | |
| `LIBC-stdio-fflush-01` | 7.21.5.2 | fflush | fn | Flushes buffered output | Positive | supported | stdout | |
| `LIBC-stdio-setvbuf-01` | 7.21.5.5,7.21.5.6 | setvbuf / setbuf | fn | Sets stream buffering mode | Positive | partial | stdout | |
| `LIBC-stdio-fread-01` | 7.21.8.1 | fread | fn | Reads up to `nmemb` elements | Positive | partial | exit | entity undeclared — not yet implemented |
| `LIBC-stdio-fwrite-01` | 7.21.8.2 | fwrite | fn | Writes `nmemb` elements | Positive | partial | exit | entity undeclared — not yet implemented |
| `LIBC-stdio-fgetc-01` | 7.21.7 | fgetc / getc / getchar / ungetc | fn | Character input | Positive | partial | exit | |
| `LIBC-stdio-fputc-01` | 7.21.7 | fputc / putc / putchar | fn | Character output | Positive | supported | stdout | |
| `LIBC-stdio-fgets-01` | 7.21.7.2 | fgets | fn | Reads a line (bounded) | Positive | partial | exit | |
| `LIBC-stdio-fputs-01` | 7.21.7.4 | fputs / puts | fn | Writes a string (`puts` appends newline) | Positive | supported | stdout | |
| `LIBC-stdio-fseek-01` | 7.21.9 | fseek / ftell / rewind / fgetpos / fsetpos | fn | Stream positioning | Positive | partial | exit | |
| `LIBC-stdio-remove-01` | 7.21.4 | remove / rename / tmpfile / tmpnam | fn | File-system operations | Positive | partial | exit | `tmpfile`/`tmpnam` partial |
| `LIBC-stdio-error-01` | 7.21.10 | clearerr / feof / ferror / perror | fn | Stream error/EOF state | Positive | partial | exit | |
| `LIBC-stdio-flush-exit-01` | 5.1.2.3p6 | (buffered streams) | fn | Line-buffered output flushes at normal termination / `exit()` | Positive | supported | stdout | atexit-routed flush |

## `<math.h>` (7.12)

Each base function with an `f`/`l` suffix denotes the `float`/`long double` variant; `long double`
aliases `double` (`docs/spec.md`). Accuracy of results is implementation-defined (5.2.4.2.2p6 —
`docs/spec.md` gap on libm accuracy).

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-math-types-01` | 7.12p2 | float_t / double_t | type | Evaluation types defined | B-impl | partial | static-assert | `FLT_EVAL_METHOD` 0 → `float`/`double` |
| `LIBC-math-HUGE_VAL-01` | 7.12p3 | HUGE_VAL / HUGE_VALF / HUGE_VALL / INFINITY / NAN | obj-macro | Defined; `INFINITY`/`NAN` are IEEE values | Positive | supported | exit | |
| `LIBC-math-FP_classes-01` | 7.12p6 | FP_INFINITE / FP_NAN / FP_NORMAL / FP_SUBNORMAL / FP_ZERO | obj-macro | Distinct ICE classification values | Positive | supported | static-assert | |
| `LIBC-math-errhandling-01` | 7.12p9 | math_errhandling / MATH_ERRNO / MATH_ERREXCEPT | obj-macro | Defined; error-reporting mechanism | B-impl | supported | static-assert | `docs/spec.md`: errno-based → `math_errhandling == MATH_ERRNO` |
| `LIBC-math-fpclassify-01` | 7.12.3.1 | fpclassify | fn-macro | Classifies a floating value | Positive | supported | exit | |
| `LIBC-math-isnan-01` | 7.12.3.4 | isnan | fn-macro | True for NaN | Positive | supported | exit | |
| `LIBC-math-isinf-01` | 7.12.3.3 | isinf | fn-macro | True for ±∞ | Positive | supported | exit | |
| `LIBC-math-isfinite-01` | 7.12.3.2 | isfinite | fn-macro | True for finite values | Positive | supported | exit | |
| `LIBC-math-isnormal-01` | 7.12.3.5 | isnormal | fn-macro | True for normal values | Positive | supported | exit | |
| `LIBC-math-signbit-01` | 7.12.3.6 | signbit | fn-macro | Sign bit (incl. signed zero) | Positive | supported | exit | |
| `LIBC-math-trig-01` | 7.12.4 | sin / cos / tan / asin / acos / atan / atan2 (+ f/l) | fn | Trigonometric functions | Positive | partial | exit | accuracy impl-defined |
| `LIBC-math-hyper-01` | 7.12.5 | sinh / cosh / tanh / asinh / acosh / atanh (+ f/l) | fn | Hyperbolic functions | Positive | partial | exit | |
| `LIBC-math-exp-01` | 7.12.6 | exp / exp2 / expm1 / log / log2 / log10 / log1p / frexp / ldexp / ilogb / logb / modf / scalbn / scalbln (+ f/l) | fn | Exponential/logarithmic functions | Positive | partial | exit | |
| `LIBC-math-pow-01` | 7.12.7 | pow / sqrt / cbrt / hypot / fabs (+ f/l) | fn | Power and absolute-value functions | Positive | partial | exit | `sqrt`/`fabs` map to wasm ops; entity undeclared — not yet implemented |
| `LIBC-math-erf-01` | 7.12.8 | erf / erfc / lgamma / tgamma (+ f/l) | fn | Error and gamma functions | Positive | deferred | none | not implemented |
| `LIBC-math-round-01` | 7.12.9 | ceil / floor / trunc / round / lround / llround / nearbyint / rint / lrint / llrint (+ f/l) | fn | Nearest-integer functions | Positive | supported | exit | pure-C bit-twiddling (no wasm rounding intrinsics); `round` halves away from zero, `rint`/`nearbyint` to-nearest-even |
| `LIBC-math-fmod-01` | 7.12.10 | fmod / remainder / remquo (+ f/l) | fn | Remainder functions | Positive | partial | exit | |
| `LIBC-math-manip-01` | 7.12.11 | copysign / nan / nextafter / nexttoward (+ f/l) | fn | Manipulation functions | Positive | partial | exit | |
| `LIBC-math-fmax-01` | 7.12.12 | fdim / fmax / fmin (+ f/l) | fn | Max/min/positive-difference | Positive | supported | exit | NaN treated as missing data (F.10.9) |
| `LIBC-math-fma-01` | 7.12.13 | fma (+ f/l) | fn | Fused multiply-add | Positive | partial | exit | |
| `LIBC-math-compare-01` | 7.12.14 | isgreater / isgreaterequal / isless / islessequal / islessgreater / isunordered | fn-macro | Quiet comparison macros | Positive | supported | exit | |
| `LIBC-math-errno-01` | 7.12.1 | (all) | fn | Domain/range errors set `errno`/raise FE exceptions per `math_errhandling` | B-impl | partial | none | `docs/spec.md`: errno-based; **accuracy gap** |
| `LIBC-math-FP_CONTRACT-01` | 7.12.2 | FP_CONTRACT | pragma | Controls expression contraction | B-impl | deferred | none | `docs/spec.md`: no contraction |

## `<time.h>` (7.27)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-time-types-01` | 7.27.1 | clock_t / time_t / struct tm / struct timespec / size_t | type | Time types defined | Positive | partial | static-assert | |
| `LIBC-time-macros-01` | 7.27.1 | CLOCKS_PER_SEC / TIME_UTC / NULL | obj-macro | Defined | Positive | partial | static-assert | |
| `LIBC-time-time-01` | 7.27.2.4 | time | fn | Current calendar time | Positive | supported | exit | via `sys_proc.clock_gettime` |
| `LIBC-time-clock-01` | 7.27.2.1 | clock | fn | Processor time | Positive | partial | exit | |
| `LIBC-time-difftime-01` | 7.27.2.2 | difftime | fn | Difference of two `time_t` in seconds | Positive | supported | exit | |
| `LIBC-time-mktime-01` | 7.27.2.3 | mktime | fn | `struct tm` → `time_t` (normalizing) | Positive | partial | exit | |
| `LIBC-time-timespec_get-01` | 7.27.2.5 | timespec_get | fn | Fills a `timespec` for a time base | Positive | partial | exit | |
| `LIBC-time-gmtime-01` | 7.27.3.3,7.27.3.4 | gmtime / localtime | fn | `time_t` → broken-down time | Positive | partial | exit | |
| `LIBC-time-asctime-01` | 7.27.3.1,7.27.3.2 | asctime / ctime | fn | Broken-down/`time_t` → string | Positive | partial | exit | |
| `LIBC-time-strftime-01` | 7.27.3.5 | strftime | fn | Formats broken-down time per a format string | Positive | partial | exit | "C" locale |

## `<inttypes.h>` (7.8)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-inttypes-imaxdiv_t-01` | 7.8p1 | imaxdiv_t | type | Result type for `imaxdiv` | Positive | supported | static-assert | |
| `LIBC-inttypes-PRI-01` | 7.8.1 | PRIdN / PRIiN / PRIoN / PRIuN / PRIxN / PRIXN (+ LEAST/FAST/MAX/PTR) | obj-macro | `printf` format-specifier string macros | Positive | supported | static-assert | string literals concatenable |
| `LIBC-inttypes-SCN-01` | 7.8.1 | SCNdN / SCNiN / SCNoN / SCNuN / SCNxN (+ LEAST/FAST/MAX/PTR) | obj-macro | `scanf` format-specifier string macros | Positive | partial | static-assert | scanf family deferred |
| `LIBC-inttypes-imaxabs-01` | 7.8.2.1,7.8.2.2 | imaxabs / imaxdiv | fn | Greatest-width absolute value / division | Positive | supported | exit | |
| `LIBC-inttypes-strtoimax-01` | 7.8.2.3 | strtoimax / strtoumax | fn | Parse greatest-width integers | Positive | supported | exit | |
| `LIBC-inttypes-wcstoimax-01` | 7.8.2.4 | wcstoimax / wcstoumax | fn | Wide-string parse to greatest-width integers | Positive | deferred | none | wide strings deferred |

## `<complex.h>` (7.3)

Complex arithmetic — a conditional feature wvmcc does not provide (`__STDC_NO_COMPLEX__` defined,
`docs/spec.md`). Enumerated for completeness.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-complex-header-01` | 7.3.1 | complex / _Complex_I / imaginary / _Imaginary_I / I | obj-macro | Header/macros (conditional feature) | Positive | by-design | none | `__STDC_NO_COMPLEX__`; not provided |
| `LIBC-complex-CMPLX-01` | 7.3.9.3 | CMPLX / CMPLXF / CMPLXL | fn-macro | Build a complex value | Positive | by-design | none | unsupported |
| `LIBC-complex-manip-01` | 7.3.9 | creal / cimag / conj / cproj / carg / cabs (+ f/l) | fn | Manipulation functions | Positive | by-design | none | unsupported |
| `LIBC-complex-math-01` | 7.3.5–7.3.8 | csin / ccos / cexp / clog / cpow / csqrt / … (+ f/l) | fn | Complex trig/exp/power | Positive | by-design | none | unsupported |
| `LIBC-complex-CX_LIMITED_RANGE-01` | 7.3.4 | CX_LIMITED_RANGE | pragma | Range-reduction pragma | B-impl | by-design | none | unsupported |

## `<fenv.h>` (7.6)

Floating-point environment access — deferred; WasmVM exposes no FP status/control state.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-fenv-types-01` | 7.6 | fenv_t / fexcept_t / FE_DFL_ENV / FE_* exceptions / FE_* rounding | type/obj-macro | Defined | Positive | deferred | none | no FP environment on WasmVM |
| `LIBC-fenv-except-01` | 7.6.2 | feclearexcept / feraiseexcept / fegetexceptflag / fesetexceptflag / fetestexcept | fn | FP exception flags | Positive | deferred | none | |
| `LIBC-fenv-round-01` | 7.6.3 | fegetround / fesetround | fn | Rounding-mode control | Positive | deferred | none | round-to-nearest fixed |
| `LIBC-fenv-env-01` | 7.6.4 | fegetenv / fesetenv / feholdexcept / feupdateenv | fn | Whole-environment control | Positive | deferred | none | |
| `LIBC-fenv-access-01` | 7.6.1 | FENV_ACCESS | pragma | Access pragma | B-impl | deferred | none | |

## `<locale.h>` (7.11)

Only the `"C"` locale is supported (`docs/spec.md`).

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-locale-lconv-01` | 7.11 | struct lconv / LC_ALL / LC_COLLATE / LC_CTYPE / LC_MONETARY / LC_NUMERIC / LC_TIME / NULL | type/obj-macro | Defined | Positive | partial | static-assert | struct lconv / LC_* assertions pass; `_Static_assert(NULL == 0)` is by-design not an ICE — wvmcc keeps `NULL` type-safe as `((void*)0)`, and a pointer cast is not an ICE operand (6.6p6) |
| `LIBC-locale-setlocale-01` | 7.11.1.1 | setlocale | fn | Selects a locale; only `"C"`/`""` succeed | Positive | partial | exit | "C" locale only |
| `LIBC-locale-localeconv-01` | 7.11.2.1 | localeconv | fn | Returns `"C"`-locale numeric formatting | Positive | partial | exit | |
| `LIBC-locale-other-01` | 7.11.1.1 | setlocale | fn | Locales other than `"C"` are implementation-defined | B-impl | by-design | none | only `"C"` |

## `<setjmp.h>` (7.13)

Nonlocal jumps — unsupported (`docs/spec.md`).

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-setjmp-jmp_buf-01` | 7.13 | jmp_buf | type | Defined | Positive | deferred | none | unsupported |
| `LIBC-setjmp-setjmp-01` | 7.13.1.1 | setjmp | fn-macro | Saves the calling environment | Positive | deferred | none | |
| `LIBC-setjmp-longjmp-01` | 7.13.2.1 | longjmp | fn | Nonlocal jump back to a saved environment | Positive | deferred | none | |

## `<signal.h>` (7.14)

No asynchronous signals on WasmVM; handler installation is deferred.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-signal-types-01` | 7.14 | sig_atomic_t / SIG_DFL / SIG_ERR / SIG_IGN / SIGABRT / SIGFPE / SIGILL / SIGINT / SIGSEGV / SIGTERM | type/obj-macro | Defined | Positive | supported | static-assert | values defined |
| `LIBC-signal-signal-01` | 7.14.1.1 | signal | fn | Install a signal handler | Positive | deferred | none | no async signals |
| `LIBC-signal-raise-01` | 7.14.2.1 | raise | fn | Raise a signal | Positive | deferred | none | |

## `<stdatomic.h>` (7.17)

Atomics — a conditional feature, deferred (`__STDC_NO_ATOMICS__` defined, `docs/spec.md`).

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-stdatomic-header-01` | 7.17.1 | ATOMIC_*_LOCK_FREE / ATOMIC_VAR_INIT / ATOMIC_FLAG_INIT | obj-macro | Header/macros (conditional feature) | Positive | deferred | none | `__STDC_NO_ATOMICS__` |
| `LIBC-stdatomic-types-01` | 7.17.6 | atomic_* types / atomic_flag / memory_order | type | Atomic types and memory order | Positive | deferred | none | |
| `LIBC-stdatomic-ops-01` | 7.17.7,7.17.8 | atomic_load / store / exchange / compare_exchange_* / fetch_* / flag_test_and_set | fn-macro | Atomic operations | Positive | deferred | none | |
| `LIBC-stdatomic-fence-01` | 7.17.4 | atomic_thread_fence / atomic_signal_fence / kill_dependency | fn-macro | Fences and dependency control | Positive | deferred | none | |

## `<tgmath.h>` (7.25)

Type-generic math — deferred; requires `_Generic` (deferred) and `<complex.h>` (by-design).

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-tgmath-real-01` | 7.25p2 | sin / cos / sqrt / pow / … (type-generic) | fn-macro | Dispatch a real-math call by argument type | Positive | deferred | none | needs `_Generic` |
| `LIBC-tgmath-complex-01` | 7.25p2 | (complex-capable generic macros) | fn-macro | Dispatch real/complex by type | Positive | by-design | none | complex unsupported |

## `<threads.h>` (7.26)

Threads — by-design absent (single-threaded, `__STDC_NO_THREADS__` defined, `docs/spec.md`).

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-threads-header-01` | 7.26.1 | thread_local / ONCE_FLAG_INIT / TSS_DTOR_ITERATIONS / thrd_* enums | obj-macro | Header/macros (conditional feature) | Positive | by-design | none | `__STDC_NO_THREADS__` |
| `LIBC-threads-thrd-01` | 7.26.5 | thrd_create / thrd_join / thrd_exit / … | fn | Thread management | Positive | by-design | none | single-threaded |
| `LIBC-threads-mtx-01` | 7.26.4 | mtx_init / mtx_lock / mtx_unlock / … | fn | Mutexes | Positive | by-design | none | |
| `LIBC-threads-cnd-01` | 7.26.3 | cnd_init / cnd_wait / cnd_signal / … | fn | Condition variables | Positive | by-design | none | |
| `LIBC-threads-tss-01` | 7.26.6 | tss_create / tss_get / tss_set / call_once | fn | Thread-specific storage | Positive | by-design | none | |

## `<uchar.h>` (7.28)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-uchar-types-01` | 7.28 | char16_t / char32_t / mbstate_t / size_t | type | Defined | Positive | partial | static-assert | |
| `LIBC-uchar-conv-01` | 7.28.1 | mbrtoc16 / c16rtomb / mbrtoc32 / c32rtomb | fn | Restartable multibyte↔wide conversion | Positive | deferred | none | unsupported |

## `<wchar.h>` (7.29)

Wide-character utilities — deferred.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-wchar-types-01` | 7.29.1 | wchar_t / wint_t / mbstate_t / struct tm / WEOF / WCHAR_MIN / WCHAR_MAX / NULL | type/obj-macro | Defined | Positive | partial | static-assert | |
| `LIBC-wchar-io-01` | 7.29.2,7.29.3 | fwprintf / wprintf / swprintf / fwscanf / fgetwc / fputwc / … | fn | Wide formatted/character I/O | Positive | deferred | none | |
| `LIBC-wchar-numconv-01` | 7.29.4.1 | wcstod / wcstol / wcstoul / … | fn | Wide numeric conversions | Positive | deferred | none | |
| `LIBC-wchar-string-01` | 7.29.4 | wcscpy / wcscat / wcscmp / wcslen / wcschr / wcsstr / … | fn | Wide string utilities | Positive | deferred | none | |
| `LIBC-wchar-conv-01` | 7.29.6 | mbrtowc / wcrtomb / mbsrtowcs / wcsrtombs / btowc / wctob | fn | Restartable multibyte↔wide conversions | Positive | deferred | none | |
| `LIBC-wchar-wmem-01` | 7.29.4.4,7.29.4.5 | wmemcpy / wmemmove / wmemcmp / wmemset / wmemchr | fn | Wide memory utilities | Positive | deferred | none | |

## `<wctype.h>` (7.30)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-wctype-types-01` | 7.30.1 | wctype_t / wctrans_t / wint_t / WEOF | type/obj-macro | Defined | Positive | partial | static-assert | |
| `LIBC-wctype-class-01` | 7.30.2 | iswalnum / iswalpha / … / iswxdigit / iswctype / wctype | fn | Wide character classification | Positive | deferred | none | |
| `LIBC-wctype-map-01` | 7.30.3 | towlower / towupper / towctrans / wctrans | fn | Wide character case mapping | Positive | deferred | none | |

## Future library directions (7.31)

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|
| `LIBC-future-01` | 7.31 | (informative) | obj-macro | Future library directions are informative — no normative test | B-impl | by-design | none | informative clause |

<!-- libc.md complete: §§7.2–7.31. -->
