# Language Standard Catalog (C17 Clause 6 + Clauses 4–5)

Test-case catalog for wvmcc's compliance with the C17 **language**. Column definitions, the ID
scheme, and all value vocabularies are in [`README.md`](README.md). Expected behavior derives from
the C17 standard and `docs/spec.md` — never from another compiler.

Schema: **ID · Spec § · Test case · Category · Status · Verify · Notes**.

> **Build progress.** Complete — Clause 4 (Conformance), Clause 5 (Environment), and Clause 6
> (Language) §§6.2–6.11. The companion library catalog is in [`libc.md`](libc.md).

---

## Clause 4 — Conformance

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-4-01` | 4p4 | `#error` in a non-skipped group must fail translation (non-zero exit) | Negative | supported | compile-fail | pp `#error` in a non-skipped group is rejected with a diagnostic and non-zero exit (verified) |
| `LANG-4-02` | 4p4 | `#error` inside a skipped (`#if 0`) group does **not** fail translation | Positive | supported | exit | conditional inclusion skips it; also unit-xref `pp_directives_test` |
| `LANG-4-03` | 4p6 | The freestanding-required headers (`<float.h> <iso646.h> <limits.h> <stdalign.h> <stdarg.h> <stdbool.h> <stddef.h> <stdint.h> <stdnoreturn.h>`) are each includable in `-ffreestanding` | Positive | supported | static-assert | all nine headers present; the `(int)(NULL == 0)` ICE check is omitted by design (NULL is `((void*)0)`; a pointer cast is not an ICE operand, 6.6p6) — NULL is checked via a static initializer instead |

## Clause 5 — Environment

### 5.1.1.2 Translation phases

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-5.1.1.2-01` | 5.1.1.2p1(1) | Phase 1: trigraph replacement + end-of-line normalization | Positive | supported | unit-xref | `pp_basic_test` (trigraphs, CRLF) |
| `LANG-5.1.1.2-02` | 5.1.1.2p1(2) | Phase 2: backslash–newline line splicing | Positive | supported | unit-xref | `pp_basic_test` |
| `LANG-5.1.1.2-03` | 5.1.1.2p1(3) | Phase 3: decompose into pp-tokens; each comment → one space | Positive | supported | unit-xref | `pp_basic_test`, `pp_tokenizer_*` |
| `LANG-5.1.1.2-04` | 5.1.1.2p1(4) | Phase 4: directives executed, macros expanded, `_Pragma` evaluated, `#include` recursive | Positive | partial | unit-xref | `pp_macro_test`/`pp_conditional_test`/`pp_include_test`; `_Pragma` **unit gap — no test** |
| `LANG-5.1.1.2-05` | 5.1.1.2p1(5) | Phase 5: source→execution charset conversion of chars/escapes | Positive | supported | unit-xref | `pp_normalize_test` |
| `LANG-5.1.1.2-06` | 5.1.1.2p1(6) | Phase 6: adjacent string-literal concatenation | Positive | supported | unit-xref | `pp_concat_tests` |
| `LANG-5.1.1.2-07` | 5.1.1.2p1(7) | Phase 7: pp-tokens → tokens; syntactic/semantic analysis | Positive | supported | unit-xref | `tests/unit/parser/*` |
| `LANG-5.1.1.2-08` | 5.1.1.2p1(8) | Phase 8: external references resolved; linked into a program image | Positive | supported | exit | linker (M2-L); a multi-symbol link runs on WasmVM |
| `LANG-5.1.1.2-09` | 5.1.1.2p1(4) | A UCN formed by `##` token pasting is undefined | B-undef | supported | none | documentation |
| `LANG-5.1.1.2-10` | 5.1.1.2p1(3) | Retention vs single-space replacement of non-newline whitespace runs | B-impl | supported | none | `docs/spec.md`: comment→1 space, runs collapsed |
| `LANG-5.1.1.2-11` | 5.1.1.2p1(1) | Mapping of physical multibyte chars to the source charset | B-impl | by-design | none | `docs/spec.md`: input assumed UTF-8 |

### 5.1.1.3 Diagnostics

The 5.1.1.3p1 requirement (a syntax-rule or constraint violation must produce ≥1 diagnostic) is
discharged by the concrete **Negative** rows throughout this catalog — no umbrella row. wvmcc now
reflects every diagnostic in a non-zero exit code (verified across syntax, constraint, semantic,
and `#error`/`_Static_assert` errors), so `compile-fail` rows are runnable.

### 5.1.2 Execution environments

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-5.1.2.1-01` | 5.1.2.1p1 | Freestanding startup function name/type is implementation-defined | B-impl | supported | exit | `docs/spec.md`: entry configurable; crt0 start-wrapper calls `main`; WasmVM invokes the module start function |
| `LANG-5.1.2.1-02` | 5.1.2.1p2 | Effect of program termination in a freestanding env is implementation-defined | B-impl | supported | exit | `docs/spec.md`: returns to WasmVM; `sys_proc.exit` sets the exit code |
| `LANG-5.1.2.2.1-01` | 5.1.2.2.1p1 | `int main(void)` form is accepted and run | Positive | supported | exit | |
| `LANG-5.1.2.2.1-02` | 5.1.2.2.1p1 | `int main(int argc, char *argv[])` form is accepted | Positive | supported | exit | argv via WasmVM `sys_proc.argc/argv`; ABI partial |
| `LANG-5.1.2.2.1-03` | 5.1.2.2.1p2 | `argc` nonnegative, `argv[argc]` null, argv strings modifiable | Positive | deferred | exit | hosted arg-passing not fully wired |
| `LANG-5.1.2.2.1-04` | 5.1.2.2.1p1 | Other (implementation-defined) startup forms | B-impl | by-design | none | `docs/spec.md`: only `main`/`_start` entry |
| `LANG-5.1.2.2.1-05` | 5.1.2.2.1p2 | Values of `argv[0..argc-1]` (program name) are implementation-defined | B-impl | deferred | none | WasmVM `argv[0]` = module path |
| `LANG-5.1.2.2.3-01` | 5.1.2.2.3p1 | `return n;` from `main` is equivalent to `exit(n)` — exit code is `n` | Positive | supported | exit | crt0 wraps `main`→`exit`; observed via WasmVM exit code |
| `LANG-5.1.2.2.3-02` | 5.1.2.2.3p1 | Reaching the closing `}` of `main` returns 0 | Positive | supported | exit | crt0 default 0 |
| `LANG-5.1.2.2.3-03` | 5.1.2.2.3p1 | `main` whose return type is not compatible with `int` → termination status unspecified | B-unspec | by-design | none | wvmcc requires `int main` |
| `LANG-5.1.2.3-01` | 5.1.2.3p2 | Side effects (volatile access, object/file modification) are sequenced per the abstract machine | Positive | supported | exit | basic sequencing supported; volatile codegen partial |
| `LANG-5.1.2.3-02` | 5.1.2.3p6 | At termination, data written to files equals abstract-semantics output | Positive | supported | stdout | stdio flush-at-exit |
| `LANG-5.1.2.3-03` | 5.1.2.3p6 | Unbuffered/line-buffered output appears promptly (7.21.3 dynamics) | Positive | supported | stdout | line-buffered stdout flush |
| `LANG-5.1.2.3-04` | 5.1.2.3p7 | What constitutes an interactive device is implementation-defined | B-impl | deferred | none | no interactive devices on WasmVM |
| `LANG-5.1.2.3-05` | 5.1.2.3p3 | Multiple unsequenced side effects on one scalar (`i = i++`) is undefined | B-undef | supported | none | documentation; see Annex C sequence points |
| `LANG-5.1.2.4-01` | 5.1.2.4p1 | Whether a freestanding program may have >1 thread is implementation-defined | B-impl | by-design | none | `docs/spec.md`: single-threaded; `<threads.h>` deferred |
| `LANG-5.1.2.4-02` | 5.1.2.4 | A data race on a non-atomic object is undefined | B-undef | deferred | none | unreachable until threads exist; placeholder |

### 5.2.1 Character sets

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-5.2.1-01` | 5.2.1p3 | Basic source/execution set members present; decimal digits contiguous (`'9'-'0'==9`) | Positive | supported | static-assert | |
| `LANG-5.2.1-02` | 5.2.1p1 | Values of execution-charset members are implementation-defined | B-impl | supported | static-assert | `docs/spec.md`: UTF-8 (`'A'==65`, …) |
| `LANG-5.2.1-03` | 5.2.1p2 | Null character (all-zero byte) exists and terminates strings (`'\0'==0`) | Positive | supported | static-assert | |
| `LANG-5.2.1-04` | 5.2.1p3 | A stray character outside identifier/literal/comment/header/never-converted-token | B-undef | supported | none | documentation |
| `LANG-5.2.1.1-01` | 5.2.1.1p1 | All nine trigraph sequences are replaced in phase 1 | Positive | supported | unit-xref | `pp_basic_test` (trigraphs) |
| `LANG-5.2.1.2-01` | 5.2.1.2p1 | Each basic-set char is one byte; null byte is not part of a multibyte char | Positive | supported | static-assert | basic chars are 1 byte; `runtime/include/limits.h` sets `MB_LEN_MAX` = 4 (UTF-8 max) |
| `LANG-5.2.1.2-02` | 5.2.1.2p1 | Encoding/meaning of extended (additional) members is locale-specific | B-impl | by-design | none | `docs/spec.md`: UTF-8, no shift states |

### 5.2.2 Character display semantics

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-5.2.2-01` | 5.2.2p2 | Alphabetic escapes `\a \b \f \n \r \t \v` map to control values (`'\n'==10`, `'\t'==9`, …) | Positive | supported | static-assert | |
| `LANG-5.2.2-02` | 5.2.2p3 | Each escape produces a unique implementation-defined `char` value | B-impl | supported | static-assert | `docs/spec.md`/ASCII: `\a`=7 `\b`=8 `\f`=12 `\n`=10 `\r`=13 `\t`=9 `\v`=11 |

### 5.2.3 Signals and interrupts

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-5.2.3-01` | 5.2.3p1 | Functions remain interruptible by signals without altering control flow/returns/auto objects | Positive | deferred | none | `<signal.h>` deferred; no signals on WasmVM |

### 5.2.4 Environmental limits

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-5.2.4.1-01` | 5.2.4.1p1 | Translate+run a program exercising the minimum limits (127 block nesting, 63 cond-incl, 1023 case labels, 4095 external idents, …) | Positive | partial | exit | compiles fast (O(2^N) operator-chain fix), loads (mem[0] sized to static data; footprint `__heap_base`=70432 B fits 2 pages), and the function-local `static char[]` init is now correct (#83). Remaining failure: a runtime out-of-bounds linear-memory access — WasmVM traps `length too long` (from invoke/memory.cpp). Static data fits mem[0], so the overflow is the runtime **stack**: wvmcc fixes the `mem[1]` stack memory at 1 page (65536 B) and never sizes/grows it to the frame, so the stress test's large frames overflow it. Confirmed by minimal repro (`char buf[70000]` alone traps `length too long`). Reproduces identically on fixed WasmVM and x86-64 CI |
| `LANG-5.2.4.1-02` | 5.2.4.1p1 | wvmcc's actual translation limits | B-impl | by-design | none | `docs/spec.md`: no fixed limits; bounded by host memory |
| `LANG-5.2.4.2.1-01` | 5.2.4.2.1p1 | `<limits.h>` macros are `#if`-usable ICEs, magnitudes ≥ standard minimums, correct sign | Positive | supported | static-assert | per-macro values in `libc.md` |
| `LANG-5.2.4.2.1-02` | 5.2.4.2.1 | Actual integer limit values (`CHAR_BIT`, `INT_MAX`, `LONG_MAX`, …) | B-impl | supported | static-assert | `docs/spec.md` LP64: `int` 32-bit, `long` 64-bit; detail in `libc.md` |
| `LANG-5.2.4.2.1-03` | 5.2.4.2.1p2 | `char` signedness fixes `CHAR_MIN`/`CHAR_MAX` | B-impl | supported | static-assert | `docs/spec.md`: signed `char` default → `CHAR_MIN==SCHAR_MIN` |
| `LANG-5.2.4.2.2-01` | 5.2.4.2.2p7 | `<float.h>` integer macros are `#if`-usable ICEs; magnitudes ≥ minimums | Positive | supported | static-assert | detail in `libc.md` |
| `LANG-5.2.4.2.2-02` | 5.2.4.2.2 | Actual floating characteristics (`FLT_RADIX`, mantissa/exp, `*_EPSILON`, `*_MAX/MIN`) | B-impl | supported | static-assert | `docs/spec.md`: IEEE-754 binary32/64; `long double` aliases `double`; detail in `libc.md` |
| `LANG-5.2.4.2.2-03` | 5.2.4.2.2p8,p9 | `FLT_ROUNDS` and `FLT_EVAL_METHOD` values | B-impl | supported | static-assert | wvmcc: round-to-nearest (1), eval method 0 |
| `LANG-5.2.4.2.2-04` | 5.2.4.2.2p10 | Subnormal support (`FLT_HAS_SUBNORM`, …) | B-impl | supported | static-assert | IEEE-754 → present (1) |
| `LANG-5.2.4.2.2-05` | 5.2.4.2.2p4 | Sign of zero/NaN/infinity may be unspecified where unsigned | B-unspec | supported | none | IEEE-754 signed zero; documentation |
| `LANG-5.2.4.2.2-06` | 5.2.4.2.2p6 | Accuracy of floating ops and `<math.h>`/`<complex.h>` results is implementation-defined | B-impl | partial | none | IEEE-754 ops exact; libm accuracy unstated — **spec.md gap** |

---

## 6.2 Concepts

### 6.2.1 Scopes of identifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.2.1-01` | 6.2.1p3 | A `goto` reaches a label declared anywhere in the same function (function scope) | Positive | supported | exit | |
| `LANG-6.2.1-02` | 6.2.1p4 | A block-scope declaration hides an outer same-name declaration; the outer is restored after the block | Positive | supported | exit | |
| `LANG-6.2.1-03` | 6.2.1p4 | A block-scope identifier is not visible after its block ends | Negative | supported | compile-fail | |
| `LANG-6.2.1-04` | 6.2.1p4 | Function-prototype-scope parameter names do not leak past the declarator | Positive | supported | exit | |
| `LANG-6.2.1-05` | 6.2.1p7 | A struct/union/enum tag is in scope just after it appears (self-referential pointer member) | Positive | supported | exit | |
| `LANG-6.2.1-06` | 6.2.1p7 | An enumeration constant is usable from just after its defining enumerator | Positive | supported | exit | |

### 6.2.2 Linkages of identifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.2.2-01` | 6.2.2p3 | A file-scope object/function with `static` has internal linkage (not visible to other TUs) | Positive | supported | exit | two-TU link test |
| `LANG-6.2.2-02` | 6.2.2p4 | `extern` reusing a prior visible declaration keeps the prior linkage | Positive | supported | exit | |
| `LANG-6.2.2-03` | 6.2.2p5 | A file-scope object with no storage-class specifier has external linkage (shared across TUs) | Positive | supported | exit | two-TU link test |
| `LANG-6.2.2-04` | 6.2.2p5 | A function with no storage-class specifier has external linkage | Positive | supported | exit | |
| `LANG-6.2.2-05` | 6.2.2p6 | A block-scope object without `extern` has no linkage (distinct per scope) | Positive | supported | exit | |
| `LANG-6.2.2-06` | 6.2.2p7 | The same identifier with both internal and external linkage in one TU is undefined | B-undef | supported | none | documentation |

### 6.2.3 Name spaces of identifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.2.3-01` | 6.2.3p1 | A tag and an ordinary identifier of the same spelling coexist (`struct foo; int foo;`) | Positive | supported | exit | |
| `LANG-6.2.3-02` | 6.2.3p1 | A label name and an ordinary identifier of the same spelling coexist | Positive | supported | exit | |
| `LANG-6.2.3-03` | 6.2.3p1 | Each struct/union has a separate member name space (same member name in two structs) | Positive | supported | exit | |

### 6.2.4 Storage durations of objects

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.2.4-01` | 6.2.4p3 | Static storage duration: a file-scope/`static` object lives for the whole program, initialized once before startup | Positive | supported | exit | |
| `LANG-6.2.4-02` | 6.2.4p5,p6 | Automatic storage: a block object is created on entry, a new instance per recursion | Positive | supported | exit | |
| `LANG-6.2.4-03` | 6.2.4p6 | A `static` block-scope object keeps its value across calls (single instance) | Positive | supported | exit | |
| `LANG-6.2.4-04` | 6.2.4p2 | Referring to an object outside its lifetime is undefined; a pointer to it becomes indeterminate | B-undef | supported | none | documentation |
| `LANG-6.2.4-05` | 6.2.4p4 | `_Thread_local` gives thread storage duration | Positive | deferred | none | threads deferred (`docs/spec.md`) |
| `LANG-6.2.4-06` | 6.2.4p4 | Indirect access to a thread-duration object from another thread is implementation-defined | B-impl | deferred | none | threads deferred |
| `LANG-6.2.4-07` | 6.2.4p7 | A VLA object's lifetime runs from its declaration to leaving the scope | Positive | deferred | none | VLAs deferred (`docs/spec.md`) |
| `LANG-6.2.4-08` | 6.2.4p8 | Modifying an object with temporary lifetime is undefined | B-undef | partial | none | documentation |

### 6.2.5 Types

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.2.5-01` | 6.2.5p2 | `_Bool` stores the values 0 and 1 | Positive | supported | static-assert | |
| `LANG-6.2.5-02` | 6.2.5p3 | A `char` holds any basic-execution-set member as a nonnegative value | Positive | supported | static-assert | |
| `LANG-6.2.5-03` | 6.2.5p3 | The value of a non-basic character stored in a `char` is implementation-defined | B-impl | supported | static-assert | `docs/spec.md`: signed `char` default |
| `LANG-6.2.5-04` | 6.2.5p5 | `signed char` and plain `char` occupy the same storage; plain `int` spans `INT_MIN..INT_MAX` | Positive | supported | static-assert | |
| `LANG-6.2.5-05` | 6.2.5p4 | The five standard signed integer types have wvmcc's LP64 sizes | Positive | supported | static-assert | `docs/spec.md`: `char`1 `short`2 `int`4 `long`8 `long long`8 |
| `LANG-6.2.5-06` | 6.2.5p4 | Implementation-defined extended integer types | B-impl | by-design | none | none provided |
| `LANG-6.2.5-07` | 6.2.5p9 | Unsigned arithmetic wraps modulo 2ᴺ (cannot overflow) | Positive | supported | exit | |
| `LANG-6.2.5-08` | 6.2.5p10 | Three real floating types; value sets `float ⊆ double ⊆ long double` | Positive | partial | static-assert | `docs/spec.md`: `long double` aliases `double` |
| `LANG-6.2.5-09` | 6.2.5p11 | `_Complex` types are a conditional feature (need not be supported) | B-impl | by-design | none | `docs/spec.md`: `_Complex` deferred; `__STDC_NO_COMPLEX__` |
| `LANG-6.2.5-10` | 6.2.5p15 | `char`, `signed char`, `unsigned char` are three distinct types; `char` behaves as one of the other two | B-impl | supported | static-assert | `docs/spec.md`: signed `char` default |
| `LANG-6.2.5-11` | 6.2.5p16 | An enumeration's constants are integer constant values; each enum is a distinct type | Positive | supported | static-assert | |
| `LANG-6.2.5-12` | 6.2.5p19 | `void` is an incomplete object type that cannot be completed (`sizeof(void)` rejected) | Negative | supported | compile-fail | |
| `LANG-6.2.5-13` | 6.2.5p20 | Derived types (array, struct, union, function, pointer) are constructible, recursively | Positive | supported | exit | |
| `LANG-6.2.5-14` | 6.2.5p27 | `_Atomic(T)` designates an atomic type (conditional feature) | B-impl | deferred | none | atomics deferred; `__STDC_NO_ATOMICS__` |
| `LANG-6.2.5-15` | 6.2.5p28 | `void*` and a pointer-to-char share representation/alignment; pointers to compatible types share representation | Positive | supported | static-assert | |

### 6.2.6 Representations of types

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.2.6.1-01` | 6.2.6.1p2,p4 | Non-bit-field objects are `n × CHAR_BIT` contiguous bits, copyable through `unsigned char[n]` (`memcpy`) | Positive | supported | exit | |
| `LANG-6.2.6.1-02` | 6.2.6.1p3 | `unsigned char` and unsigned bit-fields use a pure binary representation | Positive | supported | static-assert | |
| `LANG-6.2.6.1-03` | 6.2.6.1p4 | Number/order/encoding of bytes is implementation-defined | B-impl | supported | exit | `docs/spec.md`: little-endian; observe via `unsigned char` view |
| `LANG-6.2.6.1-04` | 6.2.6.1p5 | Reading a trap representation via a non-character lvalue is undefined | B-undef | supported | none | documentation; two's-complement integers have no trap reps |
| `LANG-6.2.6.1-05` | 6.2.6.1p6,p7 | Struct/union padding bytes take unspecified values | B-unspec | supported | none | documentation |
| `LANG-6.2.6.1-06` | 6.2.6.1p9 | Atomic loads/stores use `memory_order_seq_cst` | Positive | deferred | none | atomics deferred |
| `LANG-6.2.6.2-01` | 6.2.6.2p1,p2 | Integer object representation: value bits, (no) padding bits, sign bit | B-impl | supported | static-assert | `docs/spec.md`: two's complement, no padding bits |
| `LANG-6.2.6.2-02` | 6.2.6.2p2 | Signed integers use two's complement (one of the three permitted) | B-impl | supported | exit | `docs/spec.md`: two's complement |
| `LANG-6.2.6.2-03` | 6.2.6.2p3 | Whether negative zeros are generated/stored is unspecified | B-unspec | by-design | none | two's complement: no negative zero |
| `LANG-6.2.6.2-04` | 6.2.6.2p4 | If negative zeros are unsupported, `& | ^ ~ << >>` producing one is undefined | B-undef | supported | none | two's complement → not applicable; documentation |
| `LANG-6.2.6.2-05` | 6.2.6.2p5 | An all-zero bit pattern represents value 0 for any integer type | Positive | supported | static-assert | |
| `LANG-6.2.6.2-06` | 6.2.6.2p6 | Width = precision + sign bit (unsigned: width == precision) | B-impl | supported | static-assert | `docs/spec.md`: no padding bits |

### 6.2.7 Compatible type and composite type

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.2.7-01` | 6.2.7p1 | Identical types are compatible; cross-TU struct/union/enum compatibility by tag + members | Positive | supported | exit | two-TU link test |
| `LANG-6.2.7-02` | 6.2.7p2 | An incompatible redeclaration of one identifier within a TU is rejected | Negative | supported | compile-fail | unit-xref `sema_decl_compat_test` |
| `LANG-6.2.7-03` | 6.2.7p2 | Two declarations of the same object/function with incompatible type across TUs is undefined | B-undef | supported | none | documentation |
| `LANG-6.2.7-04` | 6.2.7p3 | The composite type of two compatible types (array size from the sized one; merged prototype) | Positive | supported | exit | |
| `LANG-6.2.7-05` | 6.2.7p3 | Composite of VLA-sized array types | Positive | deferred | none | VLAs deferred |

### 6.2.8 Alignment of objects

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.2.8-01` | 6.2.8p1 | Each complete object type has an implementation-defined alignment | B-impl | supported | static-assert | `docs/spec.md`: `char`1 `short`2 `int`/`float`4 `long`/`double`8 |
| `LANG-6.2.8-02` | 6.2.8p2 | Fundamental alignments (≤ `_Alignof(max_align_t)`) are supported for all storage durations | Positive | supported | static-assert | `docs/spec.md`: `max_align_t` = 8; `_Alignof(type)` folds in an ICE |
| `LANG-6.2.8-03` | 6.2.8p1 | `_Alignas` requests a stricter alignment | Positive | supported | static-assert | `_Alignof(obj)` reports the `_Alignas` override (#81) |
| `LANG-6.2.8-04` | 6.2.8p3 | Extended (over-)alignment beyond `max_align_t` | B-impl | supported | static-assert | `_Alignas`/`_Alignof` of objects in ICE (#81) |

---

## 6.3 Conversions

### 6.3.1 Arithmetic operands

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.3.1.1-01` | 6.3.1.1p2 | Integer promotions: `char`/`short`/`_Bool`/bit-fields promote to `int` (or `unsigned int`), preserving value | Positive | supported | exit | |
| `LANG-6.3.1.1-02` | 6.3.1.1p1 | Integer conversion rank ordering is observable (`char + char` evaluates as `int`) | Positive | supported | exit | |
| `LANG-6.3.1.1-03` | 6.3.1.1p3 | Whether plain `char` holds negative values is implementation-defined (affects promotion sign) | B-impl | supported | static-assert | `docs/spec.md`: signed `char` default |
| `LANG-6.3.1.2-01` | 6.3.1.2p1 | Converting a scalar to `_Bool` gives 0 if it compares equal to 0, else 1 (NaN → 1) | Positive | supported | exit | |
| `LANG-6.3.1.3-01` | 6.3.1.3p1 | An in-range integer-to-integer conversion preserves value | Positive | supported | exit | |
| `LANG-6.3.1.3-02` | 6.3.1.3p2 | Conversion to an unsigned type wraps modulo (max+1) | Positive | supported | exit | |
| `LANG-6.3.1.3-03` | 6.3.1.3p3 | Conversion to a signed type of an unrepresentable value is implementation-defined (or raises an impl-defined signal) | B-impl | supported | exit | `docs/spec.md`: two's-complement truncation, no signal |
| `LANG-6.3.1.4-01` | 6.3.1.4p1 | Real-float → integer truncates toward zero | Positive | supported | exit | |
| `LANG-6.3.1.4-02` | 6.3.1.4p1 | Real-float → integer when the integral part is unrepresentable is undefined | B-undef | supported | none | documentation; WasmVM `trunc` may trap on out-of-range |
| `LANG-6.3.1.4-03` | 6.3.1.4p2 | Integer → real-float: exact if representable, else nearest (choice implementation-defined) | B-impl | supported | exit | `docs/spec.md`: round-to-nearest |
| `LANG-6.3.1.5-01` | 6.3.1.5p1 | `double` → `float`: exact if representable, else nearest (choice implementation-defined) | B-impl | supported | exit | round-to-nearest |
| `LANG-6.3.1.5-02` | 6.3.1.5p1 | A floating conversion of an out-of-range value is undefined | B-undef | supported | none | documentation |
| `LANG-6.3.1.6-01` | 6.3.1.6p1 | Complex → complex converts real and imaginary parts | Positive | by-design | none | `_Complex` unsupported (`docs/spec.md`) |
| `LANG-6.3.1.7-01` | 6.3.1.7 | Real ↔ complex conversions | Positive | by-design | none | `_Complex` unsupported |
| `LANG-6.3.1.8-01` | 6.3.1.8p1 | UAC: if one operand is `long double`/`double`/`float`, the other converts to it | Positive | partial | exit | `docs/spec.md`: `long double` aliases `double` |
| `LANG-6.3.1.8-02` | 6.3.1.8p1 | UAC, same signedness: lesser-rank operand converts to the greater rank (`int + long → long`) | Positive | supported | exit | |
| `LANG-6.3.1.8-03` | 6.3.1.8p1 | UAC, unsigned rank ≥ signed rank: signed converts to unsigned (`int + unsigned → unsigned`) | Positive | supported | exit | |
| `LANG-6.3.1.8-04` | 6.3.1.8p1 | UAC, signed type represents all unsigned values: unsigned converts to signed (`long + unsigned int → long`) | Positive | supported | exit | LP64: 64-bit `long` holds 32-bit `unsigned` |
| `LANG-6.3.1.8-05` | 6.3.1.8p1 | UAC, otherwise: both convert to the unsigned version of the signed type | Positive | supported | exit | |
| `LANG-6.3.1.8-06` | 6.3.1.8p2 | Floating operands/results may carry extra range/precision; the types are unchanged | B-impl | supported | none | `FLT_EVAL_METHOD` 0; documentation |

### 6.3.2 Other operands

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.3.2.1-01` | 6.3.2.1p2 | Lvalue conversion: reading an lvalue yields the stored value as a non-lvalue | Positive | supported | exit | |
| `LANG-6.3.2.1-02` | 6.3.2.1p1 | A non-modifiable lvalue (const/array/incomplete) as an assignment target is rejected | Negative | supported | compile-fail | |
| `LANG-6.3.2.1-03` | 6.3.2.1p3 | Array → pointer decay (except as `sizeof`/`&` operand or string-literal array initializer) | Positive | supported | exit | |
| `LANG-6.3.2.1-04` | 6.3.2.1p4 | Function designator → pointer-to-function decay | Positive | supported | exit | |
| `LANG-6.3.2.1-05` | 6.3.2.1p1 | Evaluating an lvalue that designates no object is undefined | B-undef | supported | none | documentation |
| `LANG-6.3.2.1-06` | 6.3.2.1p2 | Reading an uninitialized auto object that could have had `register` storage (address never taken) is undefined | B-undef | supported | none | documentation |
| `LANG-6.3.2.2-01` | 6.3.2.2p1 | A `void` expression's value is discarded and cannot be used or converted (except to `void`) | Negative | supported | compile-fail | |
| `LANG-6.3.2.3-01` | 6.3.2.3p1 | `void*` ↔ object-pointer round-trip compares equal to the original | Positive | supported | exit | |
| `LANG-6.3.2.3-02` | 6.3.2.3p2 | Adding a qualifier (`T*` → `const T*`) preserves the pointer value | Positive | supported | exit | |
| `LANG-6.3.2.3-03` | 6.3.2.3p3 | A null pointer constant (`0` or `(void*)0`) yields a null pointer unequal to any object/function | Positive | supported | exit | |
| `LANG-6.3.2.3-04` | 6.3.2.3p4 | Null pointers of any types compare equal | Positive | supported | exit | |
| `LANG-6.3.2.3-05` | 6.3.2.3p5 | Integer → pointer conversion is implementation-defined | B-impl | supported | exit | `docs/spec.md`: integer = linear-memory address |
| `LANG-6.3.2.3-06` | 6.3.2.3p6 | Pointer → integer conversion is implementation-defined; unrepresentable result is undefined | B-impl | supported | exit | LP64: pointer fits `i64`/`intptr_t` |
| `LANG-6.3.2.3-07` | 6.3.2.3p7 | Object-pointer → differently-typed object-pointer; a `char*` points to the lowest byte and increments cover the object | Positive | supported | exit | |
| `LANG-6.3.2.3-08` | 6.3.2.3p7 | Converting to a pointer with stricter alignment than the object permits is undefined | B-undef | supported | none | documentation |
| `LANG-6.3.2.3-09` | 6.3.2.3p8 | Function-pointer ↔ differently-typed function-pointer round-trip compares equal | Positive | supported | exit | `docs/spec.md`: tagged-i64 funcref model |
| `LANG-6.3.2.3-10` | 6.3.2.3p8 | Calling a function through a pointer of incompatible type is undefined | B-undef | supported | none | documentation; `call_indirect` type-mismatch traps on WasmVM |

---

## 6.4 Lexical elements

Runtime-unobservable lexical conformance is verified at the front-end (`Verify=unit-xref`); rows
cite the covering `tests/unit/` test, and gaps where no unit test exists are flagged.

### 6.4 / 6.4.1 General & Keywords

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4-01` | 6.4p4 | Longest-match (maximal-munch) tokenization (`x+++++y` → `x ++ ++ + y`) | Positive | supported | unit-xref | `pp_tokenizer_punct_test` |
| `LANG-6.4-02` | 6.4p2 | A pp-token that cannot become a keyword/identifier/constant/string-literal/punctuator (stray char) is rejected | Negative | partial | compile-fail | **unit gap — no test**; ties to 5.2.1p3 |
| `LANG-6.4-03` | 6.4p3 | A `'` or `"` matching only the "single non-white-space character" category is undefined | B-undef | supported | none | documentation |
| `LANG-6.4.1-01` | 6.4.1p1,p2 | All C17 keywords are recognized and reserved | Positive | supported | unit-xref | `lexer_keyword_test` |
| `LANG-6.4.1-02` | 6.4.1p2 | `_Imaginary` is reserved | Positive | partial | unit-xref | recognized; imaginary types unsupported |

### 6.4.2 Identifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4.2-01` | 6.4.2.1p1,p2 | Identifier formation (nondigit + digits, leading `_`, case-sensitive) | Positive | supported | unit-xref | `lexer_identifier_test`, `pp_tokenizer_identifier_test` |
| `LANG-6.4.2-02` | 6.4.2.1p3 | UCNs (`\u`,`\U`) in identifiers within the D.1 allowed ranges | Positive | supported | unit-xref | `pp_tokenizer_identifier_test` |
| `LANG-6.4.2-03` | 6.4.2.1p3 | A UCN designating a disallowed (D.2 / initial) character is rejected | Negative | partial | compile-fail | **unit gap — no test** |
| `LANG-6.4.2-04` | 6.4.2.1p4 | A pp-token convertible to keyword-or-identifier becomes the keyword | Positive | supported | unit-xref | `lexer_keyword_test` |
| `LANG-6.4.2-05` | 6.4.2.1p5,p6 | Number of significant initial characters is implementation-defined | B-impl | supported | none | `docs/spec.md`/wvmcc: no limit (all significant) |
| `LANG-6.4.2-06` | 6.4.2.1p6 | Identifiers differing only in non-significant characters is undefined | B-undef | by-design | none | all characters significant → N/A |
| `LANG-6.4.2-07` | 6.4.2.1p3 | Which extended (multibyte) characters are permitted in identifiers is implementation-defined | B-impl | partial | none | UTF-8 |
| `LANG-6.4.2.2-01` | 6.4.2.2p1 | `__func__` is implicitly declared as the enclosing function's name | Positive | partial | stdout | **unit gap — no test** |

### 6.4.3 Universal character names

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4.3-01` | 6.4.3p1,p4 | `\u` (4 hex) and `\U` (8 hex) name characters by short identifier | Positive | supported | unit-xref | `pp_tokenizer_identifier_test`, `pp_normalize_test` |
| `LANG-6.4.3-02` | 6.4.3p2 | A UCN naming a disallowed character (`< 00A0` except `$ @ \``, or `D800–DFFF`) is rejected | Negative | partial | compile-fail | **unit gap — no test** |

### 6.4.4 Constants

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4.4.1-01` | 6.4.4.1p1–p5 | Decimal/octal/hex integer constants with all suffix forms get the correct value | Positive | supported | unit-xref | `lexer_integer_test`, `pp_tokenizer_ppnumber_test` |
| `LANG-6.4.4.1-02` | 6.4.4.1p5 | An integer constant's type is the first in its list that holds the value (LP64 ranks) | Positive | supported | static-assert | `lexer_integer_test` |
| `LANG-6.4.4.1-03` | 6.4.4.1p2,p6 | A constant value representable by no listed type (and no extended type) is rejected | Negative | partial | compile-fail | **unit gap — no test**; no extended integer types |
| `LANG-6.4.4.2-01` | 6.4.4.2p1–p4 | Decimal & hex floating constants; `f`/`F`→`float`, `l`/`L`→`long double`, none→`double` | Positive | supported | unit-xref | `lexer_floating_test`, `pp_tokenizer_ppnumber_test` |
| `LANG-6.4.4.2-02` | 6.4.4.2p3 | A non-exactly-representable floating constant rounds (correctly rounded when `FLT_RADIX` is a power of 2) | B-impl | supported | exit | round-to-nearest |
| `LANG-6.4.4.2-03` | 6.4.4.2p5 | Floating-constant conversion raises no execution-time exception | Positive | supported | exit | |
| `LANG-6.4.4.3-01` | 6.4.4.3p2 | An enumeration constant has type `int` | Positive | supported | static-assert | |
| `LANG-6.4.4.4-01` | 6.4.4.4p1–p11 | Character constants: plain & `L`/`u`/`U`-prefixed; simple/octal/hex escapes; value & type | Positive | supported | unit-xref | `lexer_char_test`, `pp_tokenizer_char_test` |
| `LANG-6.4.4.4-02` | 6.4.4.4p10 | A plain integer character constant has type `int`; single-char value per execution charset (`'A'==65`) | Positive | supported | static-assert | |
| `LANG-6.4.4.4-03` | 6.4.4.4p10 | A multi-character constant (`'ab'`) has an implementation-defined value | B-impl | supported | static-assert | `docs/spec.md`/wvmcc: defined packing |
| `LANG-6.4.4.4-04` | 6.4.4.4p9 | An octal/hex escape value out of range for the type is rejected | Negative | partial | compile-fail | **unit gap — no test** |
| `LANG-6.4.4.4-05` | 6.4.4.4p11 | Wide char constants (`L'x'`,`u'x'`,`U'x'`) have `wchar_t`/`char16_t`/`char32_t` | Positive | supported | unit-xref | `pp_tokenizer_char_test` |

### 6.4.5 String literals

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4.5-01` | 6.4.5p1–p6 | String literals: plain & `u8`/`u`/`U`/`L`-prefixed; escapes; element types | Positive | supported | unit-xref | `pp_tokenizer_string_test`, `pp_normalize_test` |
| `LANG-6.4.5-02` | 6.4.5p5 | Phase 6: adjacent identically/compatibly-prefixed literals concatenate | Positive | supported | unit-xref | `pp_concat_tests` |
| `LANG-6.4.5-03` | 6.4.5p2 | Adjacent literals mixing a wide and a UTF-8 literal are rejected | Negative | supported | unit-xref | `pp_concat_tests` (incompatible prefix) |
| `LANG-6.4.5-04` | 6.4.5p6 | A string literal initializes a static array with an appended zero terminator | Positive | supported | exit | |
| `LANG-6.4.5-05` | 6.4.5p7 | Whether identical string literals are distinct is unspecified; modifying one is undefined | B-undef | supported | none | documentation |
| `LANG-6.4.5-06` | 6.4.5p5 | Concatenability/treatment of differently-prefixed wide literals is implementation-defined | B-impl | partial | none | |

### 6.4.6 Punctuators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4.6-01` | 6.4.6p1 | All punctuators are recognized (single- and multi-character) | Positive | supported | unit-xref | `pp_tokenizer_punct_test`, `lexer_punctuator_test` |
| `LANG-6.4.6-02` | 6.4.6p3 | The six digraphs `<: :> <% %> %: %:%:` behave as `[ ] { } # ##` | Positive | supported | unit-xref | `pp_tokenizer_punct_test` |

### 6.4.7 Header names

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4.7-01` | 6.4.7p1,p2 | `<...>` and `"..."` header names are recognized in `#include` | Positive | supported | unit-xref | `pp_include_test` |
| `LANG-6.4.7-02` | 6.4.7p3 | `' \ " // /*` between `<>` (or `' \ // /*` between `""`) is undefined | B-undef | supported | none | documentation |
| `LANG-6.4.7-03` | 6.4.7p2 | Mapping of a header name to a header/source file is implementation-defined | B-impl | supported | none | `docs/spec.md`: search-path order |

### 6.4.8 Preprocessing numbers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4.8-01` | 6.4.8p1–p3 | pp-number lexical form (covers all integer/float constant tokens, `1Ex`, `0x1p-2`) | Positive | supported | unit-xref | `pp_tokenizer_ppnumber_test` |
| `LANG-6.4.8-02` | 6.4.8p4 | A pp-number that is not a valid constant after phase-7 conversion is rejected | Negative | partial | compile-fail | **unit gap — no test** |

### 6.4.9 Comments

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.4.9-01` | 6.4.9p1,p2 | Block `/* */` (non-nesting) and line `//` comments are removed; markers inside string/char literals are inert | Positive | supported | unit-xref | `pp_basic_test` |

---

## 6.5 Expressions

### 6.5 General (sequencing, aliasing, exceptional conditions)

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5-01` | 6.5p1 | Operand value computations are sequenced before the operator's value computation | Positive | supported | exit | basic sequencing |
| `LANG-6.5-02` | 6.5p2 | An unsequenced side effect with another side effect / value computation on the same scalar is undefined (`i = i++`, `a[i++] = i`) | B-undef | supported | none | documentation |
| `LANG-6.5-03` | 6.5p4 | Bitwise operators (`~ << >> & ^ |`) have implementation-defined aspects for signed types | B-impl | supported | exit | two's complement; see 6.5.7/6.5.10–12 |
| `LANG-6.5-04` | 6.5p5 | An exceptional condition (result not representable, e.g. signed overflow) is undefined | B-undef | supported | none | `docs/spec.md`: signed overflow wraps, no trap |
| `LANG-6.5-05` | 6.5p6,p7 | Effective-type / aliasing: a stored value is accessed only through a compatible or character lvalue type | Positive | supported | exit | strict-aliasing categories |
| `LANG-6.5-06` | 6.5p7 | Accessing a stored value through an incompatible non-character lvalue type is undefined | B-undef | supported | none | documentation |
| `LANG-6.5-07` | 6.5p8 | Whether a floating expression is contracted is implementation-defined (no `FP_CONTRACT`) | B-impl | partial | none | `docs/spec.md`/wvmcc: no contraction; `FP_CONTRACT` deferred |

### 6.5.1 Primary expressions

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.1-01` | 6.5.1p2 | An identifier naming an object is an lvalue primary expression | Positive | supported | exit | |
| `LANG-6.5.1-02` | 6.5.1p3,p4 | A constant / string literal is a primary expression | Positive | supported | exit | |
| `LANG-6.5.1-03` | 6.5.1p5 | A parenthesized expression preserves type/value/lvalue-ness | Positive | supported | exit | |
| `LANG-6.5.1-04` | 6.5.1p2 | An undeclared identifier is rejected | Negative | supported | compile-fail | |
| `LANG-6.5.1.1-01` | 6.5.1.1p3 | `_Generic` selects the association compatible with the controlling expression's type | Positive | deferred | none | `_Generic` not implemented — **unit gap — no test** |
| `LANG-6.5.1.1-02` | 6.5.1.1p2 | `_Generic` constraints (>1 `default`, ambiguous/duplicate types, no match without `default`) | Negative | deferred | compile-fail | `_Generic` not implemented |
| `LANG-6.5.1.1-03` | 6.5.1.1p3 | The controlling expression of `_Generic` is not evaluated | Positive | deferred | none | |

### 6.5.2 Postfix operators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.2.1-01` | 6.5.2.1p2 | `E1[E2]` ≡ `*((E1)+(E2))` element access | Positive | supported | exit | |
| `LANG-6.5.2.1-02` | 6.5.2.1p3 | Multidimensional subscripting in row-major order | Positive | supported | exit | |
| `LANG-6.5.2.1-03` | 6.5.2.1p1 | Subscript constraint: one operand pointer-to-complete-object, the other integer | Negative | supported | compile-fail | |
| `LANG-6.5.2.2-01` | 6.5.2.2p4 | Arguments are evaluated; each parameter receives its argument's value | Positive | supported | exit | |
| `LANG-6.5.2.2-02` | 6.5.2.2p5 | The function-call value is the return value (or `void`) | Positive | supported | exit | |
| `LANG-6.5.2.2-03` | 6.5.2.2p7 | Prototype: arguments are implicitly converted as by assignment to the parameter types | Positive | supported | exit | |
| `LANG-6.5.2.2-04` | 6.5.2.2p6 | Default argument promotions (`float`→`double`, integer promotions) on trailing/variadic args | Positive | supported | exit | variadic ABI-limited |
| `LANG-6.5.2.2-05` | 6.5.2.2p11 | Direct and indirect recursion is permitted | Positive | supported | exit | |
| `LANG-6.5.2.2-06` | 6.5.2.2p1 | The called expression must be a pointer-to-function returning `void` or a complete non-array object type | Negative | supported | compile-fail | |
| `LANG-6.5.2.2-07` | 6.5.2.2p2 | Prototype: argument count must equal parameter count and each be assignable | Negative | supported | compile-fail | |
| `LANG-6.5.2.2-08` | 6.5.2.2p6 | Mismatched argument count/types for a non-prototype call is undefined | B-undef | supported | none | documentation |
| `LANG-6.5.2.2-09` | 6.5.2.2p9 | Calling through a pointer incompatible with the function's definition is undefined | B-undef | supported | none | documentation; `call_indirect` type-mismatch traps |
| `LANG-6.5.2.2-10` | 6.5.2.2p10 | Calling-function evaluations are indeterminately sequenced w.r.t. the callee (no interleave) | Positive | supported | exit | |
| `LANG-6.5.2.3-01` | 6.5.2.3p3 | `.` member access yields the member value/lvalue with qualifier propagation | Positive | supported | exit | |
| `LANG-6.5.2.3-02` | 6.5.2.3p4 | `->` member access through a pointer | Positive | supported | exit | |
| `LANG-6.5.2.3-03` | 6.5.2.3p1,p2 | Constraints: `.` operand is struct/union; `->` operand is pointer-to-struct/union; second names a member | Negative | supported | compile-fail | |
| `LANG-6.5.2.3-04` | 6.5.2.3p6 | Common-initial-sequence inspection of a union of structs (union visible) | Positive | supported | exit | |
| `LANG-6.5.2.3-05` | 6.5.2.3p3 | Reading a union member other than the one last stored reinterprets the representation (type-punning) | B-impl | supported | exit | `docs/spec.md`: little-endian defined reinterpretation |
| `LANG-6.5.2.3-06` | 6.5.2.3p5 | Accessing a member of an atomic struct/union object is undefined | B-undef | deferred | none | atomics deferred |
| `LANG-6.5.2.4-01` | 6.5.2.4p2,p3 | Postfix `++`/`--` yields the old value and increments/decrements the object | Positive | supported | exit | |
| `LANG-6.5.2.4-02` | 6.5.2.4p1 | Constraint: operand is a modifiable lvalue of real or pointer type | Negative | supported | compile-fail | |
| `LANG-6.5.2.5-01` | 6.5.2.5p3,p5 | A compound literal yields an unnamed lvalue object initialized by the list | Positive | supported | exit | |
| `LANG-6.5.2.5-02` | 6.5.2.5p5 | A file-scope compound literal has static storage; a block-scope one has automatic | Positive | supported | exit | |
| `LANG-6.5.2.5-03` | 6.5.2.5p1 | Constraint: type-name is a complete object or unknown-size array, not a VLA | Negative | supported | compile-fail | |
| `LANG-6.5.2.5-04` | 6.5.2.5p13 | `const`-qualified compound literals may share storage with equal string literals | B-unspec | partial | none | documentation |

### 6.5.3 Unary operators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.3.1-01` | 6.5.3.1p2,p3 | Prefix `++E`/`--E` ≡ `E+=1`/`E-=1`; result is the new value | Positive | supported | exit | |
| `LANG-6.5.3.1-02` | 6.5.3.1p1 | Constraint: operand is a modifiable lvalue of real/pointer type | Negative | supported | compile-fail | |
| `LANG-6.5.3.2-01` | 6.5.3.2p3 | Unary `&` yields a pointer to its operand | Positive | supported | exit | |
| `LANG-6.5.3.2-02` | 6.5.3.2p4 | Unary `*` dereferences a pointer to an lvalue/function designator | Positive | supported | exit | |
| `LANG-6.5.3.2-03` | 6.5.3.2p3 | `&*E ≡ E` and `&E1[E2] ≡ E1+E2` (operators cancel) | Positive | supported | exit | |
| `LANG-6.5.3.2-04` | 6.5.3.2p1 | Constraint: `&` operand is a function designator/lvalue, not a bit-field, not `register` | Negative | supported | compile-fail | |
| `LANG-6.5.3.2-05` | 6.5.3.2p2 | Constraint: `*` operand has pointer type | Negative | supported | compile-fail | |
| `LANG-6.5.3.2-06` | 6.5.3.2p4 | Dereferencing an invalid pointer (null, misaligned, past-lifetime) is undefined | B-undef | supported | none | documentation; null/OOB load traps on WasmVM |
| `LANG-6.5.3.3-01` | 6.5.3.3p2 | Unary `+` yields the promoted operand value | Positive | supported | exit | |
| `LANG-6.5.3.3-02` | 6.5.3.3p3 | Unary `-` yields the negative of the promoted operand | Positive | supported | exit | |
| `LANG-6.5.3.3-03` | 6.5.3.3p4 | `~E` bitwise complement of the promoted operand (`~E == max-E` for unsigned) | Positive | supported | exit | |
| `LANG-6.5.3.3-04` | 6.5.3.3p5 | `!E` logical negation ≡ `(0==E)`; result type `int` | Positive | supported | exit | |
| `LANG-6.5.3.3-05` | 6.5.3.3p1 | Constraints: `+`/`-` arithmetic; `~` integer; `!` scalar | Negative | supported | compile-fail | |
| `LANG-6.5.3.4-01` | 6.5.3.4p2,p4 | `sizeof` gives object/type byte size (`sizeof(char)==1`; arrays = total; struct includes padding) | Positive | supported | static-assert | |
| `LANG-6.5.3.4-02` | 6.5.3.4p3 | `_Alignof(type)` gives the alignment requirement; operand not evaluated | Positive | supported | static-assert | unit-xref `sema_alignas_test` |
| `LANG-6.5.3.4-03` | 6.5.3.4p1 | Constraints: `sizeof` not on function/incomplete type/bit-field; `_Alignof` not on function/incomplete | Negative | supported | compile-fail | |
| `LANG-6.5.3.4-04` | 6.5.3.4p2 | `sizeof` of a VLA evaluates its operand at run time | Positive | deferred | none | VLAs deferred |
| `LANG-6.5.3.4-05` | 6.5.3.4p5 | The value of `sizeof`/`_Alignof` is implementation-defined; the type is `size_t` | B-impl | supported | static-assert | `docs/spec.md` LP64: `size_t` = `i64` |

### 6.5.4 Cast operators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.4-01` | 6.5.4p5 | `(type)expr` converts the value to the named unqualified type | Positive | supported | exit | |
| `LANG-6.5.4-02` | 6.5.4p2,p3,p4 | Constraints: cast source/target scalar (or `void`); no pointer↔floating casts | Negative | supported | compile-fail | |
| `LANG-6.5.4-03` | 6.5.4p6 | A cast removes any extra range/precision | Positive | supported | exit | |

### 6.5.5 Multiplicative operators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.5-01` | 6.5.5p3,p4 | `*` product and `/` quotient (truncation toward zero), with usual arithmetic conversions | Positive | supported | exit | |
| `LANG-6.5.5-02` | 6.5.5p5,p6 | `%` remainder; `(a/b)*b + a%b == a` | Positive | supported | exit | |
| `LANG-6.5.5-03` | 6.5.5p2 | Constraints: `*`/`/` arithmetic operands; `%` integer operands | Negative | supported | compile-fail | |
| `LANG-6.5.5-04` | 6.5.5p5 | Division or remainder by zero is undefined | B-undef | supported | none | documentation; integer div-by-zero traps on WasmVM |
| `LANG-6.5.5-05` | 6.5.5p6 | `/` or `%` with a non-representable quotient (`INT_MIN / -1`) is undefined | B-undef | supported | none | documentation |

### 6.5.6 Additive operators

Governs `+` and `-`: arithmetic addition/subtraction, pointer ± integer (scaled by element size),
and pointer − pointer (yielding `ptrdiff_t`). wvmcc data model is **LP64** (`docs/spec.md` §Data
Layout): `ptrdiff_t`/`size_t`/pointers are 64-bit (`i64`), `int` is 32-bit (`i32`).

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.6-01` | 6.5.6p5 | `int + int` yields the arithmetic sum | Positive | supported | exit | core arithmetic |
| `LANG-6.5.6-02` | 6.5.6p6 | `int - int` yields the arithmetic difference | Positive | supported | exit | |
| `LANG-6.5.6-03` | 6.5.6p4 | Mixed arithmetic operands undergo usual arithmetic conversions (`int + unsigned`, `int + long`) — result type/value per 6.3.1.8 | Positive | supported | exit | LP64: `int + long` → `long`/`i64` |
| `LANG-6.5.6-04` | 6.5.6p8 | `ptr + int` advances by `int * sizeof(*ptr)` (e.g. `int*` steps 4 bytes) | Positive | supported | exit | scaling offset is `i64` (LP64) |
| `LANG-6.5.6-05` | 6.5.6p8 | `int + ptr` is commutative with `ptr + int` | Positive | supported | exit | |
| `LANG-6.5.6-06` | 6.5.6p8 | `ptr - int` retreats by `int * sizeof(*ptr)` | Positive | supported | exit | |
| `LANG-6.5.6-07` | 6.5.6p8 | Pointer one-past-the-end is computable and comparable (no deref) | Positive | supported | exit | array of length one rule (p7) |
| `LANG-6.5.6-08` | 6.5.6p9 | `ptr - ptr` (same array) yields the element-index difference | Positive | supported | exit | result has type `ptrdiff_t` |
| `LANG-6.5.6-09` | 6.5.6p2 | Adding two pointers (`p + q`) is rejected | Negative | supported | compile-fail | constraint: not both arithmetic / ptr+int |
| `LANG-6.5.6-10` | 6.5.6p2 | `ptr + ptr`/`ptr + float` — non-integer added to pointer is rejected | Negative | supported | compile-fail | operand must have integer type |
| `LANG-6.5.6-11` | 6.5.6p2 | `ptr + int` where `ptr` is to an **incomplete** type (`void*`, incomplete struct) is rejected | Negative | supported | compile-fail | must point to *complete* object type |
| `LANG-6.5.6-12` | 6.5.6p3 | Subtracting pointers to **incompatible** object types is rejected | Negative | supported | compile-fail | |
| `LANG-6.5.6-13` | 6.5.6p9 | Type of `ptr - ptr` is `ptrdiff_t` — a signed integer type | B-impl | supported | static-assert | `docs/spec.md`: `ptrdiff_t` = signed 64-bit (`i64`); assert via `_Generic`/`sizeof` |
| `LANG-6.5.6-14` | 6.5.6p8 | Pointer arithmetic producing a result outside `[array, one-past-end]` | B-undef | supported | none | UB per p8. `docs/spec.md` UB policy: no trap; address computed in linear memory, wraps mod 2⁶⁴ |
| `LANG-6.5.6-15` | 6.5.6p9 | `ptr - ptr` whose result is not representable in `ptrdiff_t` | B-undef | supported | none | UB per p9. Documentation only |
| `LANG-6.5.6-16` | 6.5.6p8 | Dereferencing a one-past-the-end pointer (`*(a+N)`) | B-undef | supported | none | UB; out-of-bounds load may trap on WasmVM (`length_too_long`) — `docs/spec.md` UB policy |

---

### 6.5.7 Bitwise shift operators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.7-01` | 6.5.7p4 | `E1 << E2` left shift; unsigned result modulo 2ᴺ | Positive | supported | exit | |
| `LANG-6.5.7-02` | 6.5.7p5 | `E1 >> E2` right shift; for unsigned/nonneg-signed equals `E1 / 2^E2` | Positive | supported | exit | |
| `LANG-6.5.7-03` | 6.5.7p2 | Constraint: both operands have integer type | Negative | supported | compile-fail | |
| `LANG-6.5.7-04` | 6.5.7p3 | A shift count negative or ≥ the promoted width is undefined | B-undef | supported | none | documentation; wasm shift masks the count (no trap) |
| `LANG-6.5.7-05` | 6.5.7p4 | Signed `<<` overflow (`E1 × 2^E2` not representable) is undefined | B-undef | supported | none | documentation |
| `LANG-6.5.7-06` | 6.5.7p5 | Right shift of a negative signed value is implementation-defined | B-impl | supported | exit | `docs/spec.md`: arithmetic (sign-extending) shift |
| `LANG-6.5.7-07` | 6.5.7p3 | Result type is the promoted left operand (no usual arithmetic conversions) | Positive | supported | exit | |

### 6.5.8 Relational operators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.8-01` | 6.5.8p6 | `< > <= >=` yield 1/0 (type `int`) for arithmetic operands | Positive | supported | exit | |
| `LANG-6.5.8-02` | 6.5.8p5 | Pointer relational comparison within the same array/object | Positive | supported | exit | |
| `LANG-6.5.8-03` | 6.5.8p2 | Constraint: both operands real type, or both compatible-object pointers | Negative | supported | compile-fail | |
| `LANG-6.5.8-04` | 6.5.8p5 | Relational comparison of pointers into different objects is undefined | B-undef | supported | none | documentation |

### 6.5.9 Equality operators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.9-01` | 6.5.9p3 | `==`/`!=` yield 1/0 (type `int`) | Positive | supported | exit | |
| `LANG-6.5.9-02` | 6.5.9p5,p6 | Pointer equality (same object / null / one-past-end); null-pointer-constant comparison | Positive | supported | exit | |
| `LANG-6.5.9-03` | 6.5.9p2 | Constraint: arithmetic both, or compatible pointers, or pointer/`void*`, or pointer/null-constant | Negative | supported | compile-fail | |

### 6.5.10 – 6.5.12 Bitwise AND / exclusive OR / inclusive OR

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.10-01` | 6.5.10p4 | `&` bitwise AND with usual arithmetic conversions | Positive | supported | exit | |
| `LANG-6.5.10-02` | 6.5.10p2 | Constraint: integer operands | Negative | supported | compile-fail | |
| `LANG-6.5.11-01` | 6.5.11p4 | `^` bitwise exclusive OR | Positive | supported | exit | |
| `LANG-6.5.11-02` | 6.5.11p2 | Constraint: integer operands | Negative | supported | compile-fail | |
| `LANG-6.5.12-01` | 6.5.12p4 | `|` bitwise inclusive OR | Positive | supported | exit | |
| `LANG-6.5.12-02` | 6.5.12p2 | Constraint: integer operands | Negative | supported | compile-fail | |

### 6.5.13 – 6.5.14 Logical AND / OR

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.13-01` | 6.5.13p3 | `&&` yields 1 if both operands ≠ 0, else 0 (type `int`) | Positive | supported | exit | |
| `LANG-6.5.13-02` | 6.5.13p4 | `&&` short-circuits: RHS unevaluated when LHS == 0; sequence point between | Positive | supported | exit | |
| `LANG-6.5.13-03` | 6.5.13p2 | Constraint: scalar operands | Negative | supported | compile-fail | |
| `LANG-6.5.14-01` | 6.5.14p3 | `||` yields 1 if either operand ≠ 0, else 0 (type `int`) | Positive | supported | exit | |
| `LANG-6.5.14-02` | 6.5.14p4 | `||` short-circuits: RHS unevaluated when LHS ≠ 0; sequence point between | Positive | supported | exit | |
| `LANG-6.5.14-03` | 6.5.14p2 | Constraint: scalar operands | Negative | supported | compile-fail | |

### 6.5.15 Conditional operator

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.15-01` | 6.5.15p4 | `?:` evaluates exactly one of the 2nd/3rd operands; sequence point after the 1st | Positive | supported | exit | |
| `LANG-6.5.15-02` | 6.5.15p5 | Arithmetic 2nd/3rd operands → usual-arithmetic-conversion result type | Positive | supported | exit | |
| `LANG-6.5.15-03` | 6.5.15p6 | Pointer 2nd/3rd operands → composite/qualified result type; null-constant and `void*` rules | Positive | supported | exit | |
| `LANG-6.5.15-04` | 6.5.15p2,p3 | Constraints: 1st scalar; 2nd/3rd both arithmetic / same struct-union / void / compatible pointers / pointer+null / pointer+`void*` | Negative | supported | compile-fail | |
| `LANG-6.5.15-05` | 6.5.15p4 | `?:` as an operand of a binary operator preserves the other operand (no value-stack corruption when the condition is true) | Positive | supported | exit | |

### 6.5.16 Assignment operators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.16-01` | 6.5.16p3 | An assignment stores into the LHS object; the expression value is the LHS after assignment (not an lvalue) | Positive | supported | exit | |
| `LANG-6.5.16-02` | 6.5.16p2 | Constraint: the LHS is a modifiable lvalue | Negative | supported | compile-fail | |
| `LANG-6.5.16.1-01` | 6.5.16.1p2 | Simple `=` converts the RHS to the LHS type and stores it | Positive | supported | exit | |
| `LANG-6.5.16.1-02` | 6.5.16.1p1 | Simple-assignment constraints (arithmetic; compatible struct/union; compatible pointers with qualifier rules; `void*`; null constant; `_Bool` from pointer) | Negative | supported | compile-fail | |
| `LANG-6.5.16.1-03` | 6.5.16.1p3 | An overlapping assignment whose storage overlaps but types are incompatible is undefined | B-undef | supported | none | documentation |
| `LANG-6.5.16.2-01` | 6.5.16.2p3 | `E1 op= E2` ≡ `E1 = E1 op (E2)` with `E1` evaluated once | Positive | supported | exit | |
| `LANG-6.5.16.2-02` | 6.5.16.2p1,p2 | Compound-assignment constraints (`+=`/`-=`: pointer+integer or arithmetic; others arithmetic) | Negative | supported | compile-fail | |

### 6.5.17 Comma operator

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.5.17-01` | 6.5.17p2 | The comma operator evaluates its LHS as `void` (sequence point), then yields the RHS value/type | Positive | supported | exit | |

---

## 6.6 Constant expressions

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.6-01` | 6.6p2,p6 | Integer constant expressions evaluate at translation time (array size, enum value, bit-field width, `case` label) | Positive | supported | static-assert | |
| `LANG-6.6-02` | 6.6p3 | Constraint: no assignment/`++`/`--`/function-call/comma operators (except in an unevaluated subexpression) | Negative | supported | compile-fail | |
| `LANG-6.6-03` | 6.6p4 | Constraint: a constant expression's value must be in range for its type | Negative | supported | compile-fail | |
| `LANG-6.6-04` | 6.6p6 | ICE operand rules: only integer/enum/char constants, `sizeof`/`_Alignof`, and float constants as immediate cast operands | Negative | supported | compile-fail | unit-xref `sema_enum_test`, `static_assert_test` |
| `LANG-6.6-05` | 6.6p7,p8 | Arithmetic constant expressions are accepted in initializers | Positive | supported | static-assert | |
| `LANG-6.6-06` | 6.6p9 | Address constants (`&` of a static-duration object, a function designator, array/function decay) | Positive | supported | exit | |
| `LANG-6.6-07` | 6.6p10 | The implementation may accept other forms of constant expressions | B-impl | partial | none | `docs/spec.md`: ICE-evaluator scope |
| `LANG-6.6-08` | 6.6p11 | Short-circuit makes `2 || 1/0` a valid ICE with value 1 (no division by zero) | Positive | supported | static-assert | |

---

## 6.7 Declarations

### 6.7 General

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7-01` | 6.7p2 | A declaration declares at least a declarator, a tag, or enum members (empty declaration rejected) | Negative | supported | compile-fail | |
| `LANG-6.7-02` | 6.7p3 | A no-linkage identifier is declared at most once per scope/name-space | Negative | supported | compile-fail | |
| `LANG-6.7-03` | 6.7p3 | A typedef name may be redefined to the same type in the same scope | Positive | supported | exit | |
| `LANG-6.7-04` | 6.7p4 | Same-scope declarations of the same object/function must have compatible types | Negative | supported | compile-fail | unit-xref `sema_decl_compat_test` |
| `LANG-6.7-05` | 6.7p7 | A no-linkage object's type must be complete by the end of its declarator/init-declarator | Negative | supported | compile-fail | |

### 6.7.1 Storage-class specifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.1-01` | 6.7.1p1,p5 | `typedef extern static auto register` (and `_Thread_local`) recognized | Positive | supported | exit | `_Thread_local` deferred |
| `LANG-6.7.1-02` | 6.7.1p2 | At most one storage-class specifier (except `_Thread_local` with `static`/`extern`) | Negative | supported | compile-fail | |
| `LANG-6.7.1-03` | 6.7.1p3,p4 | `_Thread_local` block-scope needs `static`/`extern`; not on a function | Negative | deferred | compile-fail | threads deferred |
| `LANG-6.7.1-04` | 6.7.1p7 | A block-scope function declaration's only allowed storage class is `extern` | Negative | supported | compile-fail | |
| `LANG-6.7.1-05` | 6.7.1p6 | The effectiveness of a `register` request is implementation-defined | B-impl | supported | exit | `docs/spec.md`: `register` treated as `auto` |
| `LANG-6.7.1-06` | 6.7.1p6 | Taking the address of (or decaying) a `register` object is rejected | Negative | supported | compile-fail | |

### 6.7.2 Type specifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.2-01` | 6.7.2p2 | Every valid type-specifier multiset denotes the right type (`unsigned long int`, `signed char`, …) | Positive | supported | static-assert | |
| `LANG-6.7.2-02` | 6.7.2p2 | At least one type specifier per declaration; an invalid multiset is rejected | Negative | supported | compile-fail | |
| `LANG-6.7.2-03` | 6.7.2p3 | `_Complex` cannot be used when complex types are unsupported | Negative | by-design | compile-fail | `__STDC_NO_COMPLEX__` |
| `LANG-6.7.2-04` | 6.7.2p5 | Whether a bit-field `int` is signed or unsigned is implementation-defined | B-impl | partial | static-assert | `docs/spec.md`: signed |

### 6.7.2.1 Structure and union specifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.2.1-01` | 6.7.2.1p6–p8 | A struct-declaration-list declares a new struct/union type with members | Positive | supported | exit | |
| `LANG-6.7.2.1-02` | 6.7.2.1p3 | Constraint: no member of incomplete/function type (except FAM); no direct self-containment | Negative | supported | compile-fail | |
| `LANG-6.7.2.1-03` | 6.7.2.1p15 | Non-bit-field members have increasing addresses in declaration order; a struct pointer points to the first member | Positive | supported | exit | |
| `LANG-6.7.2.1-04` | 6.7.2.1p16 | A union is sized to its largest member; a union pointer points to each member | Positive | supported | exit | |
| `LANG-6.7.2.1-05` | 6.7.2.1p4 | Bit-field width is a nonnegative ICE ≤ the type width; zero width has no declarator | Negative | supported | compile-fail | |
| `LANG-6.7.2.1-06` | 6.7.2.1p5 | A bit-field type is `_Bool`/`signed int`/`unsigned int`/an impl-defined type | Negative | supported | compile-fail | |
| `LANG-6.7.2.1-07` | 6.7.2.1p10,p11 | Bit-field value semantics and packing into storage units | Positive | supported | exit | unit-xref `struct_union_test` |
| `LANG-6.7.2.1-08` | 6.7.2.1p11 | Bit-field straddle/packing and allocation order are implementation-defined; unit alignment is unspecified | B-impl | supported | exit | `docs/spec.md`: LSB-first defined layout |
| `LANG-6.7.2.1-09` | 6.7.2.1p13 | Anonymous struct/union members are members of the containing type | Positive | supported | exit | |
| `LANG-6.7.2.1-10` | 6.7.2.1p14 | Each non-bit-field member is aligned in an implementation-defined manner | B-impl | supported | static-assert | `docs/spec.md`: natural alignment |
| `LANG-6.7.2.1-11` | 6.7.2.1p18 | A flexible array member (last member of a multi-member struct, incomplete array) | Positive | supported | exit | |
| `LANG-6.7.2.1-12` | 6.7.2.1p18 | Accessing FAM elements beyond the allocation is undefined | B-undef | partial | none | documentation |
| `LANG-6.7.2.1-13` | 6.7.2.1p8 | A struct/union with no named members is undefined | B-undef | supported | none | documentation |

### 6.7.2.2 Enumeration specifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.2.2-01` | 6.7.2.2p3 | Enumerators have type `int`; sequential values, `=` overrides, duplicates allowed | Positive | supported | exit | |
| `LANG-6.7.2.2-02` | 6.7.2.2p2 | An enumerator value must be an ICE representable as `int` | Negative | supported | compile-fail | unit-xref `sema_enum_test` |
| `LANG-6.7.2.2-03` | 6.7.2.2p4 | The enum's compatible integer type is implementation-defined | B-impl | supported | static-assert | `docs/spec.md`: `int` unless out of range |

### 6.7.2.3 Tags

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.2.3-01` | 6.7.2.3p1 | A specific type's content is defined at most once | Negative | supported | compile-fail | |
| `LANG-6.7.2.3-02` | 6.7.2.3p2 | Two declarations of one tag must use the same `struct`/`union`/`enum` keyword | Negative | supported | compile-fail | |
| `LANG-6.7.2.3-03` | 6.7.2.3p3 | `enum identifier` (no list) is allowed only after the enum is complete | Negative | supported | compile-fail | |
| `LANG-6.7.2.3-04` | 6.7.2.3p7,p8 | Self-referential and forward-declared (incomplete) struct/union tags | Positive | supported | exit | |
| `LANG-6.7.2.3-05` | 6.7.2.3p4,p5 | Tag scoping: same tag/scope = same type; different scope or tag = distinct types | Positive | supported | exit | |

### 6.7.2.4 Atomic type specifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.2.4-01` | 6.7.2.4p2,p3 | `_Atomic(type-name)` (conditional feature) and its type-name constraints | Positive | deferred | none | atomics deferred; `__STDC_NO_ATOMICS__` |

### 6.7.3 Type qualifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.3-01` | 6.7.3p1,p6 | `const volatile restrict _Atomic` recognized; a repeated qualifier counts once | Positive | supported | exit | `_Atomic` deferred |
| `LANG-6.7.3-02` | 6.7.3p2 | Constraint: only pointer-to-object types may be `restrict`-qualified | Negative | supported | compile-fail | unit-xref `sema_restrict_test` |
| `LANG-6.7.3-03` | 6.7.3p7 | Modifying a `const`-qualified object through a non-const lvalue is undefined | B-undef | supported | none | documentation |
| `LANG-6.7.3-04` | 6.7.3p7 | A `const` object as an assignment target is rejected | Negative | supported | compile-fail | unit-xref `sema_qualifiers_test` |
| `LANG-6.7.3-05` | 6.7.3p8 | `volatile` accesses are evaluated strictly per the abstract machine | Positive | supported | exit | volatile codegen partial |
| `LANG-6.7.3-06` | 6.7.3p8 | What constitutes a `volatile` access is implementation-defined | B-impl | partial | none | `docs/spec.md`: each load/store |
| `LANG-6.7.3-07` | 6.7.3p10 | Array qualifiers qualify the element type; qualifying a function type is undefined | B-undef | supported | none | documentation |
| `LANG-6.7.3.1-01` | 6.7.3.1 | `restrict` aliasing contract: accessing a restrict object via another pointer is undefined | B-undef | partial | none | documentation; wvmcc may ignore `restrict` (permitted) |

### 6.7.4 Function specifiers

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.4-01` | 6.7.4p1,p6 | `inline` function specifier and inline-substitution suggestion | Positive | supported | exit | unit-xref `sema_inline_test` |
| `LANG-6.7.4-02` | 6.7.4p2 | Function specifiers appear only on function declarations | Negative | supported | compile-fail | |
| `LANG-6.7.4-03` | 6.7.4p7 | Inline-definition vs external-definition rules (`extern` ⇒ external definition) | Positive | supported | exit | |
| `LANG-6.7.4-04` | 6.7.4p3 | An external-linkage inline definition must not define a static-duration modifiable object or reference an internal-linkage identifier | Negative | supported | compile-fail | |
| `LANG-6.7.4-05` | 6.7.4p8 | A `_Noreturn` function does not return to its caller | Positive | supported | exit | emits trailing `unreachable` |
| `LANG-6.7.4-06` | 6.7.4p8,p9 | A `_Noreturn` function that returns is undefined (recommended diagnostic) | B-undef | supported | none | documentation |
| `LANG-6.7.4-07` | 6.7.4p6 | The extent to which inline suggestions are effective is implementation-defined | B-impl | partial | none | `docs/spec.md` |
| `LANG-6.7.4-08` | 6.7.4p4 | No function specifier on `main` (hosted) | Negative | supported | compile-fail | |
| `LANG-6.7.4-09` | 6.7.4p7 | Whether a call uses the inline or external definition is unspecified | B-unspec | partial | none | documentation |

### 6.7.5 Alignment specifier

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.5-01` | 6.7.5p6,p7 | `_Alignas(type)` ≡ `_Alignas(_Alignof(type))`; `_Alignas(const-expr)`; strictest wins | Positive | supported | static-assert | `_Alignas(type-name)` + member `_Alignas`→struct alignment in ICE (#81) |
| `LANG-6.7.5-02` | 6.7.5p2 | Constraint: `_Alignas` not with `typedef`/`register`, not on a function/bit-field | Negative | supported | compile-fail | |
| `LANG-6.7.5-03` | 6.7.5p3–p5 | Constraint: a valid (supported) alignment, not weaker than required | Negative | supported | compile-fail | |
| `LANG-6.7.5-04` | 6.7.5p8 | Inconsistent `_Alignas` across declarations of one object is undefined | B-undef | partial | none | documentation |

### 6.7.6 Declarators

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.6-01` | 6.7.6p1,p4–p6 | Pointer/array/function declarators; parenthesized declarators bind correctly | Positive | supported | exit | unit-xref `declarator_test` |
| `LANG-6.7.6.1-01` | 6.7.6.1p1,p2 | Pointer declarators; qualifier placement; pointer-type compatibility | Positive | supported | exit | |
| `LANG-6.7.6.2-01` | 6.7.6.2p3,p4 | Array declarators; constant-size arrays complete, `[]` incomplete | Positive | supported | exit | |
| `LANG-6.7.6.2-02` | 6.7.6.2p1 | Constraint: array size ICE > 0; element type complete non-function; `static`/qualifiers only on a parameter's outermost array | Negative | supported | compile-fail | |
| `LANG-6.7.6.2-03` | 6.7.6.2p4,p5 | VLA declarators (`[n]` non-ICE, `[*]`) | Positive | deferred | none | VLAs deferred |
| `LANG-6.7.6.2-04` | 6.7.6.2p2 | VLA scope/linkage constraints (no static/thread VLA; ordinary block/proto only) | Negative | deferred | compile-fail | VLAs deferred |
| `LANG-6.7.6.2-05` | 6.7.6.2p6 | Array-type compatibility; an incompatible-size VLA context is undefined | B-undef | deferred | none | documentation |
| `LANG-6.7.6.3-01` | 6.7.6.3p5,p10 | Function declarators/prototypes; `(void)` means no parameters | Positive | supported | exit | |
| `LANG-6.7.6.3-02` | 6.7.6.3p1–p4 | Constraints: no function/array return type; only `register` parameter storage; non-defining identifier list empty; complete param types in a definition | Negative | supported | compile-fail | |
| `LANG-6.7.6.3-03` | 6.7.6.3p7,p8 | A parameter "array of T" adjusts to "pointer to T"; "function returning T" to "pointer to function" | Positive | supported | exit | |
| `LANG-6.7.6.3-04` | 6.7.6.3p14,p15 | Old-style (identifier-list/empty) vs prototype declarator compatibility | Positive | supported | exit | |

### 6.7.7 Type names

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.7-01` | 6.7.7p2 | Type names (abstract declarators) parse for casts, `sizeof`, compound literals, `_Generic` | Positive | supported | exit | |

### 6.7.8 Type definitions

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.8-01` | 6.7.8p3 | A `typedef` introduces a synonym (not a new type), usable in declarations | Positive | supported | exit | unit-xref `sema_typedef_declarator_test` |
| `LANG-6.7.8-02` | 6.7.8p3 | A typedef name shares the ordinary-identifier name space (can be shadowed) | Positive | supported | exit | |
| `LANG-6.7.8-03` | 6.7.8p2 | A typedef of a variably modified type must have block scope | Negative | deferred | compile-fail | VLAs deferred |
| `LANG-6.7.8-04` | 6.7.8p3 | A typedef-name object is sized by the underlying type (`typedef long X; X v;` → 8 bytes) | Positive | supported | static-assert | typedef-name sized by its underlying type (e.g. `typedef long X` → 8 bytes, i64); verified |

### 6.7.9 Initialization

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.9-01` | 6.7.9p11 | Scalar initialization (optionally braced) with simple-assignment conversions | Positive | supported | exit | unit-xref `initializer_test` |
| `LANG-6.7.9-02` | 6.7.9p2 | Constraint: an initializer must not exceed the entity being initialized | Negative | supported | compile-fail | |
| `LANG-6.7.9-03` | 6.7.9p3 | Constraint: the entity type is complete (or unknown-size array), not a VLA | Negative | supported | compile-fail | |
| `LANG-6.7.9-04` | 6.7.9p4 | Constraint: static/thread initializers are constant expressions or string literals | Negative | supported | compile-fail | |
| `LANG-6.7.9-05` | 6.7.9p5 | Constraint: a block-scope extern/internal-linkage declaration has no initializer | Negative | supported | compile-fail | |
| `LANG-6.7.9-06` | 6.7.9p10 | Static/thread objects without explicit initializer are zero-initialized (null pointer / zero / recursive) | Positive | supported | exit | |
| `LANG-6.7.9-07` | 6.7.9p10 | An automatic object without initializer has indeterminate value | B-undef | supported | none | documentation |
| `LANG-6.7.9-08` | 6.7.9p13 | A struct/union object initialized from a compatible-type expression | Positive | supported | exit | |
| `LANG-6.7.9-09` | 6.7.9p14,p15 | A char array from a string literal (terminator if room); wide-char arrays | Positive | supported | exit | unit-xref `initializer_test` |
| `LANG-6.7.9-10` | 6.7.9p17–p22 | Aggregate brace-enclosed lists; current-object order; nested braces; partial bracketing | Positive | supported | exit | unit-xref `initializer_test` |
| `LANG-6.7.9-11` | 6.7.9p6,p7,p17,p18 | Designated initializers (`[i]=`, `.member=`); out-of-order; combined | Positive | supported | exit | unit-xref `initializer_test` |
| `LANG-6.7.9-12` | 6.7.9p21 | Fewer initializers than elements → the remainder is zero-initialized | Positive | supported | exit | |
| `LANG-6.7.9-13` | 6.7.9p22 | An unknown-size array is sized by its largest designated index | Positive | supported | exit | |
| `LANG-6.7.9-14` | 6.7.9p23 | Initializer-list side-effect evaluation order is unspecified | B-unspec | supported | none | documentation |
| `LANG-6.7.9-15` | 6.7.9p19 | A later initializer overrides an earlier one for the same subobject | B-unspec | supported | exit | overridden initializer may be unevaluated |

### 6.7.10 Static assertions

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.7.10-01` | 6.7.10p3 | `_Static_assert(nonzero-ICE, "msg")` has no effect (passes) | Positive | supported | static-assert | unit-xref `sema_static_assert_test` |
| `LANG-6.7.10-02` | 6.7.10p2,p3 | `_Static_assert(0, …)` or a non-ICE controlling expression is rejected | Negative | supported | compile-fail | unit-xref `sema_static_assert_test` |

---

## 6.8 Statements and blocks

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.8-01` | 6.8p3 | A block groups declarations and statements; an auto initializer is evaluated when its declaration is reached | Positive | supported | exit | |
| `LANG-6.8-02` | 6.8p4 | There is a sequence point after each full expression | Positive | supported | exit | |
| `LANG-6.8.1-01` | 6.8.1p1 | A labeled statement `label: stmt` is a `goto` target | Positive | supported | exit | |
| `LANG-6.8.1-02` | 6.8.1p2 | `case`/`default` labels appear only inside a `switch` | Negative | supported | compile-fail | |
| `LANG-6.8.1-03` | 6.8.1p3 | Label names are unique within a function | Negative | supported | compile-fail | |
| `LANG-6.8.2-01` | 6.8.2p1 | A compound statement `{ … }` is a block mixing declarations and statements | Positive | supported | exit | |
| `LANG-6.8.3-01` | 6.8.3p2 | An expression statement evaluates its expression as `void` for side effects | Positive | supported | exit | |
| `LANG-6.8.3-02` | 6.8.3p3 | A null statement `;` performs no operation | Positive | supported | exit | |
| `LANG-6.8.4.1-01` | 6.8.4.1p2 | `if`: the substatement runs iff the controlling expression ≠ 0; `else` form | Positive | supported | exit | |
| `LANG-6.8.4.1-02` | 6.8.4.1p3 | `else` binds to the nearest preceding `if` (dangling-else) | Positive | supported | exit | |
| `LANG-6.8.4.1-03` | 6.8.4.1p1 | Constraint: an `if` controlling expression has scalar type | Negative | supported | compile-fail | |
| `LANG-6.8.4.2-01` | 6.8.4.2p5 | `switch`: control jumps to the matching `case`, else `default`, else past the body | Positive | supported | exit | |
| `LANG-6.8.4.2-02` | 6.8.4.2p3 | Constraint: `case` labels are ICEs, unique within the switch; at most one `default` | Negative | supported | compile-fail | |
| `LANG-6.8.4.2-03` | 6.8.4.2p1 | Constraint: a `switch` controlling expression has integer type | Negative | supported | compile-fail | |
| `LANG-6.8.4.2-04` | 6.8.4.2p4 | Fall-through between cases without `break` | Positive | supported | exit | |
| `LANG-6.8.5.1-01` | 6.8.5.1p1 | `while`: the controlling expression is evaluated before each body execution | Positive | supported | exit | |
| `LANG-6.8.5.2-01` | 6.8.5.2p1 | `do`: the controlling expression is evaluated after each body execution | Positive | supported | exit | |
| `LANG-6.8.5.3-01` | 6.8.5.3p1,p2 | `for(clause-1; e2; e3)`: init, pre-test `e2`, post-body `e3`; omitted `e2` is nonzero | Positive | supported | exit | |
| `LANG-6.8.5-01` | 6.8.5p2 | Constraint: an iteration controlling expression has scalar type | Negative | supported | compile-fail | |
| `LANG-6.8.5-02` | 6.8.5p3 | Constraint: a `for` declaration declares only `auto`/`register` objects | Negative | supported | compile-fail | |
| `LANG-6.8.5-03` | 6.8.5p6 | A non-constant-controlled loop with no I/O/volatile/sync may be assumed to terminate | B-impl | partial | none | `docs/spec.md`/wvmcc: no forced-progress transform |
| `LANG-6.8.6.1-01` | 6.8.6.1p2 | `goto label` jumps to the labeled statement in the enclosing function | Positive | supported | exit | |
| `LANG-6.8.6.1-02` | 6.8.6.1p1 | Constraint: `goto` targets a label in the enclosing function; no jump into a VLA scope | Negative | partial | compile-fail | VLA part deferred |
| `LANG-6.8.6.2-01` | 6.8.6.2p2 | `continue` jumps to the loop-continuation of the smallest enclosing loop (a `for` runs `e3`) | Positive | supported | exit | |
| `LANG-6.8.6.2-02` | 6.8.6.2p1 | Constraint: `continue` only within a loop body | Negative | supported | compile-fail | |
| `LANG-6.8.6.3-01` | 6.8.6.3p2 | `break` terminates the smallest enclosing `switch`/loop | Positive | supported | exit | |
| `LANG-6.8.6.3-02` | 6.8.6.3p1 | Constraint: `break` only within a `switch`/loop body | Negative | supported | compile-fail | |
| `LANG-6.8.6.4-01` | 6.8.6.4p3 | `return expr` returns the value, converted as by assignment to the return type | Positive | supported | exit | |
| `LANG-6.8.6.4-02` | 6.8.6.4p1 | Constraint: no `return expr` in a `void` function; bare `return` only in a `void` function | Negative | supported | compile-fail | |

## 6.9 External definitions

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.9-01` | 6.9p2 | Constraint: `auto`/`register` may not appear in an external declaration | Negative | supported | compile-fail | |
| `LANG-6.9-02` | 6.9p3,p5 | At most one external definition per identifier; exactly one if it is used | Negative | partial | compile-fail | cross-TU resolution at link time |
| `LANG-6.9-03` | 6.9p5 | An unused external-linkage identifier needs no definition | Positive | supported | exit | |
| `LANG-6.9.1-01` | 6.9.1p1,p11 | A function definition (declarator + body) executes its compound statement | Positive | supported | exit | |
| `LANG-6.9.1-02` | 6.9.1p2,p3,p4 | Constraints: a function-type declarator; return type `void`/complete-non-array; only `extern`/`static` storage class | Negative | supported | compile-fail | |
| `LANG-6.9.1-03` | 6.9.1p5,p6 | Constraints: prototype parameters each named (except `(void)`); identifier-list form rules | Negative | supported | compile-fail | |
| `LANG-6.9.1-04` | 6.9.1p7 | A prototype definition also serves as a prototype for later calls; parameters are complete | Positive | supported | exit | |
| `LANG-6.9.1-05` | 6.9.1p10 | On entry, VLA parameter size expressions are evaluated; args converted as by assignment | Positive | deferred | none | VLAs deferred |
| `LANG-6.9.1-06` | 6.9.1p12 | Reaching the `}` of a non-`void` function whose return value is then used is undefined | B-undef | supported | none | documentation |
| `LANG-6.9.1-07` | 6.9.1p8 | A variadic function defined without a parameter type list (old-style + `...`) is undefined | B-undef | supported | none | documentation |
| `LANG-6.9.1-08` | 6.9.1p9 | Parameter storage layout is unspecified | B-unspec | supported | none | documentation |
| `LANG-6.9.2-01` | 6.9.2p1 | A file-scope object with an initializer is an external definition | Positive | supported | exit | |
| `LANG-6.9.2-02` | 6.9.2p2 | Tentative definitions: a file-scope object without initializer/`static` collapses to one definition (init 0) | Positive | supported | exit | |
| `LANG-6.9.2-03` | 6.9.2p2 | `int i[];` with only tentative definitions completes to one zero-initialized element | Positive | supported | exit | |
| `LANG-6.9.2-04` | 6.9.2p3 | Constraint: an internal-linkage tentative definition's type must not remain incomplete | Negative | supported | compile-fail | |

---

## 6.10 Preprocessing directives

Preprocessing conformance is not runtime-observable; rows are `unit-xref` to the `tests/unit/pp/`
suite, with confirmed gaps flagged.

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.10.1-01` | 6.10.1 | `#if`/`#elif` ICE conditions; `#ifdef`/`#ifndef`; `#else`/`#endif`; nesting | Positive | supported | unit-xref | `pp_conditional_test` |
| `LANG-6.10.1-02` | 6.10.1p1 | The `defined(X)` / `defined X` operator | Positive | supported | unit-xref | `pp_conditional_test` |
| `LANG-6.10.1-03` | 6.10.1p4 | Identifiers surviving macro expansion in `#if` are replaced by 0 | Positive | supported | unit-xref | `pp_conditional_test` |
| `LANG-6.10.1-04` | 6.10.1p1 | Constraint: the controlling expression must be an integer constant expression | Negative | partial | unit-xref | **unit gap — no test** |
| `LANG-6.10.2-01` | 6.10.2 | `#include <...>` and `"..."`; nested; macro-expanded include | Positive | supported | unit-xref | `pp_include_test` |
| `LANG-6.10.2-02` | 6.10.2 | Include search-path order and quote→angle fallback are implementation-defined | B-impl | supported | none | `docs/spec.md`; `pp_include_test` |
| `LANG-6.10.3-01` | 6.10.3 | Object-like and function-like macro replacement with argument substitution | Positive | supported | unit-xref | `pp_macro_test` |
| `LANG-6.10.3-02` | 6.10.3.1 | Argument prescan/expansion | Positive | supported | unit-xref | `pp_macro_test` |
| `LANG-6.10.3-03` | 6.10.3.2 | `#` stringification | Positive | supported | unit-xref | `pp_macro_test` |
| `LANG-6.10.3-04` | 6.10.3.3 | `##` token pasting and placemarkers | Positive | supported | unit-xref | `pp_macro_test` |
| `LANG-6.10.3-05` | 6.10.3.1 | `__VA_ARGS__` variadic macros | Positive | supported | unit-xref | `pp_macro_test` |
| `LANG-6.10.3-06` | 6.10.3.4 | Rescanning / further replacement with non-recursion (paint) semantics | Positive | supported | unit-xref | `pp_macro_test` |
| `LANG-6.10.3-07` | 6.10.3.2p1 | Constraint: `#` in a function-like macro must be followed by a parameter | Negative | partial | unit-xref | **unit gap — no test** |
| `LANG-6.10.3-08` | 6.10.3.3p3 | A `##` result that is not a valid preprocessing token is undefined | B-undef | supported | none | documentation |
| `LANG-6.10.3-09` | 6.10.3p2 | Constraint: a macro redefinition must be identical | Negative | partial | unit-xref | **unit gap — no test** |
| `LANG-6.10.4-01` | 6.10.4 | `#line N` and `#line N "file"` set the line/file for diagnostics | Positive | partial | unit-xref | `pp_directives_test` (syntax; tracking unverified) |
| `LANG-6.10.5-01` | 6.10.5 | `#error` produces a diagnostic and fails translation | Negative | partial | unit-xref | `pp_directives_test`; `#error` diagnoses with non-zero exit (verified) — standard `compile-fail` row pending |
| `LANG-6.10.6-01` | 6.10.6 | `#pragma` is recognized; an unknown pragma | Positive | partial | unit-xref | `pp_directives_test` |
| `LANG-6.10.6-02` | 6.10.6 | `#pragma once` prevents re-inclusion (extension) | Positive | supported | unit-xref | `pp_pragma_once_test` |
| `LANG-6.10.6-03` | 6.10.6p2 | Standard `STDC` pragmas (`FP_CONTRACT`, `FENV_ACCESS`, `CX_LIMITED_RANGE`) | Positive | deferred | none | STDC pragmas not implemented |
| `LANG-6.10.6-04` | 6.10.6p1 | The behavior of an unrecognized pragma is implementation-defined (ignored) | B-impl | partial | none | `docs/spec.md` |
| `LANG-6.10.7-01` | 6.10.7 | A null directive (`#` alone) has no effect | Positive | partial | unit-xref | **unit gap — no test** |
| `LANG-6.10.8-01` | 6.10.8 | Predefined macros `__FILE__ __LINE__ __DATE__ __TIME__ __STDC__ __STDC_VERSION__` | Positive | supported | unit-xref | `pp_macro_test` |
| `LANG-6.10.8-02` | 6.10.8.1 | `__STDC__ == 1` and `__STDC_VERSION__ == 201710L` (C17) | Positive | supported | static-assert | |
| `LANG-6.10.8-03` | 6.10.8.3 | Conditional-feature macros (`__STDC_NO_ATOMICS__`, `__STDC_NO_COMPLEX__`, `__STDC_NO_THREADS__`, `__STDC_NO_VLA__`) | Positive | partial | static-assert | reflect deferred/by-design features |
| `LANG-6.10.8-04` | 6.10.8p2 | Constraint: predefined macros and `__LINE__`/`__FILE__` are not redefinable | Negative | partial | unit-xref | **unit gap — no test** |
| `LANG-6.10.9-01` | 6.10.9 | The `_Pragma("...")` operator | Positive | deferred | none | **unit gap — no test**; not implemented |

## 6.11 Future language directions

Clause 6.11 is informative. wvmcc targets C17 exactly; these are recorded for completeness.

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|
| `LANG-6.11-01` | 6.11.6,6.11.7 | Obsolescent non-prototype (old-style) function declarators/definitions are still accepted | Positive | supported | exit | informative; wvmcc accepts old-style |
| `LANG-6.11-02` | 6.11.1–6.11.5,6.11.8,6.11.9 | Remaining future-direction items are informative — no normative test | B-impl | by-design | none | informative clause; documented, not tested |

<!-- language.md complete: Clause 4, Clause 5, 6.2–6.11. -->
