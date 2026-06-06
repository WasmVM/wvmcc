# wvmcc Standard Test-Case Catalog

This directory catalogs, exhaustively, the test cases that exercise wvmcc's compliance with
**ISO/IEC 9899:2017 (C17, N2176)** when its output runs on **WasmVM**. It is the *contract* that a
later test-writing pass implements; this catalog contains **no test code itself**.

The catalog is split two ways, matching the standard:

| File | Standard scope |
|---|---|
| [`language.md`](language.md) | Clause 6 (Language) + runtime-relevant Clause 4 (Conformance) and Clause 5 (Environment). |
| [`libc.md`](libc.md) | Clause 7 (Library), organized by header. |

> **Design rule.** wvmcc is a *new compiler for a new target*. Expected behavior is derived from the
> **C17 standard** and wvmcc's **own design docs** (`docs/spec.md`, `docs/lowering-plan.md`,
> `docs/milestones.md`) — **never** from GCC, Clang, or any other compiler. Every
> implementation-defined expectation cites `docs/spec.md`.

---

## How a row is structured

### `language.md` — 7 columns

| ID | Spec § | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|

### `libc.md` — 8 columns

Adds **Kind**, because many library features are macros, types, or pragmas rather than functions.

| ID | Spec § | Entity | Kind | Test case | Category | Status | Verify | Notes |
|---|---|---|---|---|---|---|---|---|

Rows are grouped two-tier: a Markdown heading per standard subsection / header entity, then **one
row per discrete observable behavior** to be asserted. Thin subsections get one row; rich ones get
several.

---

## Column vocabularies

### ID
A stable, append-only handle.

- **Language:** `LANG-<§>-<NN>` — e.g. `LANG-6.5.6-03` (3rd case under §6.5.6).
- **Libc:** `LIBC-<header>-<entity>-<NN>` — e.g. `LIBC-string-memcpy-02`. `<header>` is the short
  header name without brackets/extension (`string`, `stdio`, `stdint`); `<entity>` is the standard
  identifier of **any** kind (function, macro, type, pragma).

> **IDs are permanent.** When a case is inserted between existing rows, it gets the **next unused**
> `NN` and is placed in reading order — IDs are **never renumbered**. The `NN` is an allocation tag,
> not a sort key; the row's position in the table is its reading order.

### Spec §
The normative reference. Use **paragraph-level** precision (`6.5.6p8`) where a specific paragraph is
the rule being tested (constraints, the exact sentence); section-only (`6.5.6`) otherwise.

### Entity / Kind (libc only)
`Entity` is the standard identifier. `Kind` is one of:

| Kind | Meaning |
|---|---|
| `fn` | A function. |
| `fn-macro` | A function-like macro (e.g. `assert`, `va_arg`, `isnan`). |
| `obj-macro` | An object-like / constant macro (e.g. `INT_MAX`, `SIZE_MAX`, `EOF`). |
| `type` | A typedef or tagged type (e.g. `size_t`, `int32_t`, `max_align_t`, `FILE`). |
| `pragma` | A standard pragma (e.g. `FP_CONTRACT`, `FENV_ACCESS`). |

Per **7.1.4p2**, any library function may also be provided as a macro; where the standard permits
either, `Kind` is noted `fn or fn-macro (impl choice)`.

### Category — conformance category
| Category | Meaning | How asserted |
|---|---|---|
| `Positive` | A well-formed program produces the required result. | Run / compile-time check. |
| `Negative` | An ill-formed program (constraint violation) **must be rejected**. | Compiler exits non-zero. |
| `B-impl` | Implementation-defined behavior (Annex J.3). | Assert wvmcc's **documented** choice. |
| `B-unspec` | Unspecified behavior (Annex J.1). | Assert result is one of the permitted set. |
| `B-undef` | Undefined behavior (Annex J.2). | Usually documentation only (see Verify `none`). |

Behavior (`B-*`) rows are driven by **Annex J**, filtered to the feature subset wvmcc can reach, and
placed **inline** under the standard section that governs them.

### Status — wvmcc support
| Status | Meaning |
|---|---|
| `supported` | Implemented and believed conformant — a test should pass today. |
| `partial` | Implemented with known limits (e.g. variadics are ABI-limited). |
| `deferred` | Conformant support intended **later**; a test is expected to fail/skip now. |
| `by-design` | wvmcc **intentionally and permanently** diverges from the standard. |

**Assignment precedence** (record the basis in Notes):
1. Milestone/phase doc marks the milestone **closed** → `supported`; mid-flight → `partial`.
2. `docs/spec.md` explicitly defers it (VLAs, `_Complex`, `_Atomic`, `<threads.h>`) → `deferred`.
3. Permanent intentional divergence (`long double == double`, LP64, signed-`char` default, no
   freestanding host syscalls) → `by-design`.
4. Otherwise not yet reached → `deferred`.

> Integration tests (`tests/integration/`) are **being deprecated** by this suite and are **not**
> cited as evidence. Unit tests (`tests/unit/`) **are** cited, via the `unit-xref` Verify mode.

### Verify — how the case is checked
| Verify | Mechanism |
|---|---|
| `exit` | Compile, link if needed, run on WasmVM; program returns 0 on pass, non-zero on first failed assertion (observed via `sys_proc.exit`). |
| `stdout` | As `exit`, plus the program's stdout (fd 1) must equal a fixture string. |
| `compile-fail` | Compile with `wvmcc`; the compiler must exit non-zero. Message text is **not** asserted (no stable diagnostic IDs). |
| `static-assert` | A freestanding `_Static_assert` discharges the case at compile time — **no link, no run**. Used for constant macros and type properties (`sizeof`/`_Alignof`). |
| `unit-xref` | Behavior is verified at the front-end by an existing `tests/unit/` test, cited in Notes. Used for lexical (6.4) and preprocessing (6.10) areas not observable at runtime. |
| `none` | Documentation-only row (typically `B-undef`): records wvmcc's chosen behavior with no executable assertion. |

### Notes
Free text: the `docs/spec.md` reference for `B-impl` expected values, caveats, the Status basis, and
gap flags (e.g. `unit gap — no test`, `spec.md gap`).

---

## ID → test-path convention

Test paths are **derived from the ID** (so no path column is needed). The later test-writing pass
creates, under `tests/standard/`:

- **Language:** `LANG-<§>-<NN>` → `tests/standard/language/<clause>_<name>/<§>_<name>.c`
  e.g. `LANG-6.5.6-03` → `tests/standard/language/6.5_expressions/6.5.6_additive.c`.
- **Libc:** `LIBC-<header>-<entity>-<NN>` → `tests/standard/libc/<header>/<entity>.c`
  e.g. `LIBC-string-memcpy-02` → `tests/standard/libc/string/memcpy.c`.

Rule: **one file per subsection (language) or per entity (libc)**; all rows for that
subsection/entity are asserted inside the one file, keyed by ID in comments. Heavy cases may be
split into `<name>_<NN>.c`; the README rule is the default.

The harness mirrors the existing macros in `tests/integration/CMakeLists.txt`
(`add_e2e_test` / `add_link_test` / `add_link_stdout_test`) — wiring is part of the test-writing
pass, not this catalog.

---

## Build status

| Section | State |
|---|---|
| `README.md` (this file) | complete |
| `language.md` (Clause 4, 5, 6.2–6.11) | complete |
| `libc.md` (7.2–7.31) | complete |

The catalog is exhaustive across C17 Clauses 4–7 and the reachable Annex J behavior items. The
test-writing pass materializes each row under `tests/standard/` per the ID→path convention above
(the first `static-assert` batch has landed). Known prerequisites surfaced while materializing it:

- **#80** — preprocessor/parser errors must force a non-zero exit (gates `compile-fail` rows).
- **#81** — the `_Static_assert` ICE evaluator must accept `sizeof`/`_Alignof`/casts (gates the
  type-width/alignment/signedness `static-assert` rows; only macro-value rows are live until then).
- **Missing freestanding headers** — `<float.h>`, `<iso646.h>`, `<stdalign.h>`, `<stdnoreturn.h>`
  (required per 4p6) are not yet in `runtime/include`; their rows stay `deferred`.
