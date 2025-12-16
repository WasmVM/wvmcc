# Project Milestones

This document tracks key milestones achieved so far and the upcoming work items for the freestanding C17 → Wasm compiler.

## Completed
- Repository scaffolding with CMake, tests, and CLI entrypoints
- Preprocessor Phases 1–3 (single pass)
  - Trigraph replacement
  - End-of-line normalization (CRLF/CR → LF)
  - Line splicing (backslash-newline)
  - Comment stripping (// and /* */) with correct handling inside strings/chars
  - Ensuring final newline
- Tokenizer (Streaming)
  - Streaming API (`next()`, `peek()`, range iteration); removed batch `tokenize()`
  - Whitespace and Newline
  - Punctuators (greedy longest match, including digraphs and multi-char operators)
  - String literals (prefixes: u8/u/U/L; escape handling; newline terminates)
  - Char constants (prefixes: L/u/U; escape handling: simple/octal/hex/UCN)
  - PPNumber (per 6.4.8: digit or .digit start; supports e/E and p/P with optional sign; trailing dot; hex-float forms)
  - Identifier (per 6.4.2: ASCII letters/underscore start; digits allowed after; supports UCNs \uXXXX and \UXXXXXXXX)
- Header-name recognition (6.4.7) in `#include` directives
  - Post-tokenization transform combines `<...>` or "..." into a single `HeaderName`
- Unit tests
  - Preprocessor basic tests
  - Tokenizer: whitespace, punctuators, strings, chars, pp-number, identifiers
  - Updated to use range-style iteration over streaming tokenizer
  - Header-name transform tests

## In Progress / Near-Term
- Macro processing (Phase 4)
  - `#define`, `#undef` (object-like) in M0; function-like macros, stringizing `#`, token pasting `##`, variadics in M1
  - Macro substitution rules, argument expansion; recursive expansion guards and ordering
  - See `docs/preprocessor.md` for architecture and phased plan
- Additional tokenization refinements
  - PPNumber corner cases and suffix interactions
  - Identifier UCN range validation (disallow invalid code points per spec)
  - Header-name edge cases (whitespace variations, malformed terminators)
- Single-pass Preprocessor executor
  - Consume streamed tokens; detect directives at line-start; execute `#include`, object-like macros, and basic conditionals inline.
  - Gradually expand to function-like macros, `#`, `##`, variadics.
- Diagnostics and error reporting
  - Clear messages with spans for unterminated literals and invalid sequences

## Later Milestones
- Parser for translation units (C17 subset initially)
- Semantic analysis (types, declarations, expressions)
- IR design for Wasm module generation
- Code generation to Wasm (MVP)
  - Function compilation, basic control flow, locals
  - Minimal runtime stubs for freestanding environment
- Optimizations and verification
  - Basic optimization passes
  - Validation against Wasm constraints

## Preprocessor Phases Summary
- Phase 1–3: Implemented (trigraphs, line splicing, comment removal)
- Phase 4: Directives and macro expansion (see near-term plan)
- Phase 5–6: Escapes and adjacent string literal concatenation
- Phase 7: Token conversion to language tokens
- Phase 8: Linkage

## Notes
- Freestanding target: no libc/WASI; emit Wasm modules directly.
- Unit tests should evolve alongside features; prefer focused tests with clear expectations and token dumps on failure.
