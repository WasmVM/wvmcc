# WVMCC Spec (Freestanding, WasmModule IR)

## Goals
- Freestanding C17 compiler (no libc, no WASI). No reliance on host syscalls; only pure computation and explicit imports chosen by us.
- Output format: WasmVM `WasmModule` as the final IR/module. Internal compilation can use a custom IR before lowering.
- Initial focus: correctness and clean architecture over optimizations.

## Language Scope (M0 → M2)
- M0: Core C17 expressions/statements, scalar types, pointers, arrays, functions, structs/unions/enums, qualifiers, usual arithmetic conversions, control flow, function prototypes.
- M1: Declarations + initializers (scalars/aggregates), compound literals, designators, flexible array members, bitfields (documented layout).
- M2: Variadics (ABI-limited), `_Static_assert`, `_Alignof`, `_Noreturn`, `_Generic` later.
- Deferred initially: VLAs, `_Complex`, atomics (`_Atomic`, `<stdatomic.h>`), `<threads.h>`.

## Freestanding Constraints
- No standard library. Programs may not call `printf`, `malloc`, etc.
- Entry point: configurable (e.g., `_start` or `main`) but no runtime provided by us in M0.
- Environment: linear memory only; any I/O must be via explicit imports the user defines.

## Data Layout & ABI (wasm64)
- Endianness: little-endian.
- Pointer size: 64-bit; `size_t`/`ptrdiff_t` are 64-bit. `long` remains 32-bit (LP64 not used; wasm64 only widens pointers).
- Alignment: natural (1/2/4/8). Struct layout follows wasm64/Clang-like natural alignment; document padding.
- `char` signedness: implementation-defined; default to signed (`-funsigned-char` available later).
- `long double`: initially aliased to `double`.
- Variadics: supported with wasm64-compatible ABI; validate alignment rules via tests.

## Architecture
- Frontend:
  - Preprocessor / Tokenizer: use the streaming `Tokenizer` and `Preprocessor` which emit `PPToken`s.
  - Parser: hand-written recursive descent with declarator grammar; recovery at `;`/`}`/`,`.
  - AST: typed nodes, source spans; casts inserted during semantic analysis.
  - Semantics: scope stacks (file/block/function/tag), type system (qualifiers, arrays, functions, pointers), conversions, constant folding, diagnostics.
- IR:
  - Custom IR: SSA-like blocks with simple three-address ops for expressions/control flow.
  - Passes: const-fold, copy-prop, dead-code elim (basic), canonicalization.
- Backend:
  - Final output is `WasmModule`; lowering to wasm binary is handled by `WasmVM::module_encode`.
  - Lower custom IR → WasmVM op set (`i32/i64/f32/f64`) mapped into linear memory for aggregates.
  - Calling convention: wasm64-compatible; spill to linear memory as needed.
  - No host imports in M0; expose only minimal module sections.

## Translation Phases (C17 §5.1.1.2)
- Phase 1: Source mapping + trigraphs
  - Assume UTF-8 input; replace trigraphs (`??=`→`#`, etc.); normalize EOL to `\n`; ensure final newline.
- Phase 2: Line splicing
  - Delete backslash-newline pairs; only the last backslash on a physical line is eligible.
- Phase 3: Preprocessing tokenization + comments
  - Decompose into pp-tokens and whitespace; replace comments with a single space; retain newlines; error on partial pp-token/comment at EOF.
  - Implemented via streaming `Tokenizer` (lookahead supported) and `SourceBuffer`.
- Phase 4: Directives + macro expansion
  - Implement `#define/#undef/#include/#if/#ifdef/#ifndef/#elif/#else/#endif`, `_Pragma` later; expand object/function-like macros; delete directives; includes processed recursively (phases 1–4).
  - Single-pass executor consumes streamed tokens; detects directives at line starts and executes inline.
  - See `docs/preprocessor.md` for detailed directive architecture, data structures, phased plan, and testing strategy.
- Phase 5: Char constants and strings
  - Convert escapes to execution set (UTF-8 initially).
- Phase 6: Adjacent string literal concatenation
  - Concatenate with correct prefixes (`L`, `u8`, `u`, `U`).
- Phase 7: Token conversion + compilation
  - Convert pp-tokens to language tokens; parse/type-check; lower to IR → `WasmModule`.
- Phase 8: Linkage
  - Freestanding: produce a single `WasmModule`; external refs either imports or errors.

### Preprocessing Tokens (PPToken)

Per C17 §6.4, Phase 3 decomposes the character stream into preprocessing-tokens and sequences of whitespace. We will represent both pp-tokens and whitespace as tokens, with explicit new-line tokens to preserve directive boundaries and macro spacing.

- Kind enum:
  - `header-name` (only recognized in `#include` context)
  - `identifier`
  - `pp-number`
  - `character-constant`
  - `string-literal`
  - `punctuator`
  - `whitespace` (one or more of space, HT, VT, FF)
  - `newline` (`\n` after Phase 1 normalization)
  - `other` (each non-white-space character that cannot be one of the above)

- Punctuators supported (C17 + preprocessor):
  - Single-char: `[](){}.,;&*+-~!/%<>^|?:=#` and digraphs: `<: :> <% %> %:`
  - Multi-char: `-> ++ -- << >> <= >= == != && || ... *= /= %= += -= <<= >>= &= ^= |= ##`
  - Note: `##` and `#` are only meaningful within macro contexts but still tokenized as punctuators.

- Source tracking:
  - `SourcePos { fileId, line, column, offset }`
  - `SourceSpan { begin, end }` attached to every token.

- Whitespace representation:
  - We emit `whitespace` tokens for contiguous runs of space/tab/vtab/form-feed.
  - We emit `newline` tokens for each `\n`. Comments replaced by a single space become a `whitespace` token of length 1.
  - This explicit model simplifies `#`-at-line-start detection and macro expansion spacing rules without relying on per-token trivia.

- Proposed C++ representation (conceptual):
```
enum class PPTokenKind {
  HeaderName,
  Identifier,
  PPNumber,
  CharConst,
  StringLiteral,
  Punctuator,
  Whitespace,
  Newline,
  Other
};

enum class PPPunctuator {
  LBracket, RBracket, LParen, RParen, LBrace, RBrace,
  Dot, Arrow, Comma, Semicolon, Colon, Question,
  Plus, Minus, Star, Slash, Percent, Tilde, Bang,
  Amp, Pipe, Caret,
  Lt, Gt, Le, Ge, EqEq, Ne,
  Shl, Shr, AndAnd, OrOr,
  Ellipsis,
  Assign, MulAssign, DivAssign, ModAssign, AddAssign, SubAssign,
  ShlAssign, ShrAssign, AndAssign, XorAssign, OrAssign,
  Hash, HashHash,
  Digraph_LBracket, Digraph_RBracket, Digraph_LBrace, Digraph_RBrace,
  Digraph_Hash, Digraph_HashHash
};

struct SourcePos { int fileId; int line; int column; std::size_t offset; };
struct SourceSpan { SourcePos begin; SourcePos end; };

struct PPToken {
  PPTokenKind kind;
  SourceSpan span;
  std::string lexeme;      // raw bytes after phases 1–3
  std::optional<PPPunctuator> punct; // when kind == Punctuator
  struct HeaderInfo { enum Type { Angle, Quote } type; };
  std::optional<HeaderInfo> header;   // when kind == HeaderName
};

using PPTokenStream = std::vector<PPToken>;
```

- Notes:
  - `header-name` tokens carry `HeaderInfo::type` to distinguish `<...>` vs `"..."` forms and retain raw lexeme; only recognized when the first non-space token after `#include`.
  - `pp-number` retains raw bytes; semantic interpretation (e.g., UCNs in identifiers) is deferred until later phases.
  - Explicit `newline` tokens mark logical line boundaries; `whitespace` runs may be coalesced by later stages.
  - Extended characters are UTF-8; tokenization treats them as part of identifiers/literals where allowed.

## Developer Workflows
- Build: `mkdir build && cd build && cmake .. && make -j4`
- Run: `wvmcc source.c -o out.wasm` (CLI emits WasmModule → wasm or native WasmVM format depending on support).
  - Preprocess: `src/pp` performs phases 1–4 before lexing/parsing.
- Tests:
  - Unit: C++ tests for lexer/parser/semantics/IR.
  - E2E: compile small freestanding programs (no I/O), e.g., arithmetic, branches, function calls; validate results by reading memory/return values via a test harness.
  - Diagnostics: en-US messages in M0; plan switch to zh-TW as primary in a later milestone.

## Project Conventions
- C++20 only; minimal dependencies.
- Tree:
  - `src/{lexer,parser,ast,semantics}/`
  - `src/ir/`
  - `src/pp/` (preprocessor)
  - `src/exec/`
  - `include/` (public headers)
  - `tests/{unit,e2e}/`

## Milestones
- M0: Parse/type-check core C; IR gen for expressions/statements; lower to minimal `WasmModule`; run pure computation programs in WasmVM.
  - Preprocessor M0: implement phases 1–3 fully; phase 4 with `#include`, object-like `#define`, basic `#ifdef` family. See `docs/preprocessor.md` for specifics.
- M1: Initializers/aggregates/struct layout tests; function calls across files; basic diagnostics.
  - Preprocessor M1: full macro expansion (function-like, `#`, `##`), `_Pragma`, `#line`, precise whitespace rules. See `docs/preprocessor.md`.
- M2: Variadics, `_Alignof/_Static_assert`, bitfields; improved IR passes.

## Open Topics
- Exact mapping of struct/union layout to WasmVM memory: confirm rules against Clang wasm64 and freeze.
- Module format details: emit `.wasm` via `module_encode`; consider WasmVM-native serialization if needed.
- Dependency integration: WasmVM discovered via `cmake/FindWasmVM.cmake`; keep `#include <WasmVM.hpp>`.
- CLI flags: `-target wasm64`, `-funsigned-char`, `-fno-long-double`, output selection.
 - Include search paths and file I/O: add `-I` handling; define default search order.

## Implementation-Defined Behavior (Defaults in M0)
- Data model:
  - Pointer width: 64-bit; `sizeof(void*) == 8`; `size_t`/`ptrdiff_t` are 64-bit.
  - Integer widths: `char` 8-bit, `short` 16-bit, `int` 32-bit, `long` 32-bit, `long long` 64-bit. (LP64 not used; wasm64 only widens pointers.)
  - Endianness: little-endian.
- `char` signedness:
  - Default: signed. Flag: `-funsigned-char` to switch.
- Floating point:
  - `float`/`double`: IEEE-754 binary32/binary64 → `f32`/`f64`.
  - `long double`: alias to `double` in M0. Flag: `-flong-double=64` (enforced).
- Alignment & layout:
  - Fundamental alignments: `char` 1, `short` 2, `int`/`float` 4, `long`/`double`/`long long` 8.
  - Struct/union: natural alignment with padding; follows Clang wasm64 rules.
  - Bitfields: allocation order defined left-to-right within storage unit; LSB index 0 for unsigned.
  - `max_align_t`: 8.
- Characters and execution set:
  - Source/execution charset: UTF-8 in M0.
  - Basic source/execution character sets conform to C17 §5.2.1 (letters A–Z/a–z, digits 0–9, 29 graphics, space, HT/VT/FF, and in execution: alert, BS, CR, NL). Each member fits in one byte.
  - Decimal digit values are consecutive; end-of-line is treated as a single new-line in source.
  - Null character (all-zero byte) exists in execution set and terminates strings.
  - Extended characters: allowed via UTF-8 and universal character names (UCNs) in identifiers/literals; mapping to execution set is UTF-8 in M0.
  - Escape processing applied per phases 5–6; UCNs supported in literals/identifiers as per later milestone.

### Trigraph Sequences (C17 §5.2.1.1)
Replacement is done in Phase 1, before any other processing:

| Trigraph | Replaces |
| - | - |
| `??=` | `#` |
| `??(` | `[` |
| `??/` | `\\` |
| `??)` | `]` |
| `??'` | `^` |
| `??<` | `{` |
| `??!` | `|` |
| `??>` | `}` |
| `??-` | `~` |

Any `?` not starting one of the above trigraphs is left unchanged.

- `_Bool`:
  - Size: 1 byte; `true` = 1, `false` = 0; non-zero converts to 1.
- Pointers:
  - Null representation: all-zero.
  - Function vs object pointers: distinct; no casting between them in M0.
  - `void*`: aligned to hold any object pointer.
- Variadics (ABI):
  - Default promotions: per C17 (float→double; small integers→int).
  - Calling convention: wasm64; extra args spilled per ABI definition (documented later). `va_list` representation deferred until M1/M2.
- Enums:
  - Underlying type: `int` unless values exceed range.
- `setjmp/longjmp`:
  - Unsupported in M0.
- Undefined behavior policy:
  - Signed overflow, division by zero, invalid shifts: undefined; no traps by default.
  - Optional diagnostics via future flag (e.g., `-fdiagnose-ub`).
- Preprocessor specifics:
  - Whitespace: comments replaced by a single space; newlines retained; other whitespace collapsed or retained as needed for token boundaries.
  - Includes: current working directory only in M0; `-I` added in M1.
  - Macros: object-like in M0; function-like, `#` stringize, `##` paste, `_Pragma`, `#line` in M1.
