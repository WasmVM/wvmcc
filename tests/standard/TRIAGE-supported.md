# Triage — failing `status-supported` standard tests

Snapshot **2026-07-21**. Every test under `tests/standard/` asserts ISO C17; a
failing `status-supported` test is a genuine wvmcc conformance gap (not a test
bug). The 2026-06-15 backlog below is **cleared** — every failure class was
resolved and its tracking issue closed; the suite's supported rows are 100%
green. This file now serves as the historical map of that backlog and the
dashboard-regeneration recipe.

## Current dashboard (`ctest -R '^std-'`)

| Label | Pass / Total | Notes |
|---|---|---|
| `status-supported` | **520 / 520** | "should pass today" — all green |
| `status-partial` | 1 / 1 | `goto` into nested scopes (switch-body entry still rejected) |
| `status-deferred` | 0 / 2 | expected to fail — `_Thread_local` constraints (6.7.1p3), static/thread VLA constraints (6.7.6.2p2) |
| `status-by-design` | 3 / 3 | assert wvmcc's documented divergence |
| **Total** | **524 / 526** | the 2 failures are the deferred-feature conformance signal |

Regenerate counts with: `cd build-Debug && ctest -L standard`.

## Failure classes → tracking issues (2026-06-15 backlog, all resolved)

| Class | Root cause | Issue | State |
|---|---|---|---|
| A | `_Static_assert` rejects a valid integer-constant-expression | **#81** | closed |
| B | constraint violation accepted (no diagnostic) | **#83** | closed |
| C | compiles & runs but returns the WRONG value (conversions, pointer arithmetic) | **#86** | closed |
| D | codegen emits a module WasmVM rejects (value-type-width) | **#87** | closed |
| E | parser rejects valid C17 syntax (nested compound statements; `sizeof`/cast of pointer type-names) | **#88** | closed |
| F | libc function undeclared / unimplemented | — | resolved by the M2-Libc batches |
| G | freestanding header missing (`float.h`, `inttypes.h`, `iso646.h`, `locale.h`, `signal.h`) | — | headers landed in `runtime/include` |
| H | backward / non-local `goto` unsupported | #93/#109 | closed — dispatch-loop lowering + entry-state descent; only jump into a *switch body* remains rejected (`status-partial`) |
| I | static-storage initializer constant-folding too strict | — | resolved |
| J | codegen feature unimplemented (unary/binop/conversion) | — | resolved |
| K | tentative definitions rejected as "multiple external definitions" (6.9.2); cross-TU decl-merge | **#89** / #84 | closed |
| L | const-expr in enum / errno-macro form | — | resolved by #81 |

## Method

For each `status-supported` test, compile (and, for `run`/`stdout` tests, execute on
WasmVM) with `wvmcc -ffreestanding -isystem runtime/include`, then bucket by the
first compiler error (or wrong exit code) into the classes above. Spot-checks
confirmed the failures are genuine wvmcc behaviour, not test bugs.
