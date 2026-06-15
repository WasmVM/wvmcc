# Triage — failing `status-supported` standard tests

Snapshot **2026-06-15**, after merging the #81 ICE fixes. Every test under
`tests/standard/` asserts ISO C17; a failing `status-supported` test is a genuine
wvmcc conformance gap (not a test bug). This file maps the failure **classes** to
the issues that track them; the issues are the live to-do list.

## Current dashboard (`ctest -R '^std-'`)

| Label | Pass / Total | Notes |
|---|---|---|
| `status-supported` | **218 / 367** | "should pass today" — the 149 failures are the backlog below |
| `status-partial` | 28 / 121 | known-limited areas |
| `status-deferred` | 2 / 11 | not yet implemented |
| **Total** | **248 / 499 (50%)** | true C17 conformance level |

Regenerate counts with: `cd build-Debug && ctest -R '^std-' -L status-supported`.

## Failure classes → tracking issues

| Class | Root cause | Issue | State |
|---|---|---|---|
| A | `_Static_assert` rejects a valid integer-constant-expression | **#81** | merged (symbol-free forms done; `sizeof` of a variable/member and complete-struct sizes remain) |
| B | constraint violation accepted (no diagnostic) | **#83** | partial — second batch still accepted |
| C | compiles & runs but returns the WRONG value (conversions, pointer arithmetic) | **#86** | open — largest bucket (~48) |
| D | codegen emits a module WasmVM rejects (value-type-width) | **#87** | open (~11) |
| E | parser rejects valid C17 syntax (nested compound statements; `sizeof`/cast of pointer type-names) | **#88** | open (~28) |
| F | libc function undeclared / unimplemented | — | reconciled to `status-partial` (M2-Libc) |
| G | freestanding header missing (`float.h`, `inttypes.h`, `iso646.h`, `locale.h`, `signal.h`) | *unfiled* | ~8 static-assert tests blocked on `include file not found` |
| H | backward / non-local `goto` unsupported | *unfiled* | 3 |
| I | static-storage initializer constant-folding too strict | *unfiled* | 2 |
| J | codegen feature unimplemented (unary/binop/conversion) | *unfiled* | 3 |
| K | tentative definitions rejected as "multiple external definitions" (6.9.2); cross-TU decl-merge | **#89** / #84 | open |
| L | const-expr in enum / errno-macro form | — | mostly resolved by #81 |

## Method

For each `status-supported` test, compile (and, for `run`/`stdout` tests, execute on
WasmVM) with `wvmcc -ffreestanding -isystem runtime/include`, then bucket by the
first compiler error (or wrong exit code) into the classes above. Spot-checks
confirmed the failures are genuine wvmcc behaviour, not test bugs.
