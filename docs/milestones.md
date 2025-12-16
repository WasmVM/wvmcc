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
- **Preprocessor Phase 4: Complete directive processing and macro expansion**
  - ✅ Object-like and function-like macros (`#define`, `#undef`) with recursion guards
  - ✅ Variadic macros with `__VA_ARGS__` support
  - ✅ Macro expansion with argument substitution
  - ✅ Conditional compilation (`#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`)
  - ✅ Constant expression evaluation (ConstExprParser) with full operator support
  - ✅ `defined()` operator with proper semantics (no operand expansion)
  - ✅ `#include` directives with cycle detection and search path resolution
  - ✅ Macro-replaced includes (expand then reparse header-name)
  - ✅ Nested conditional tracking and inactive region skipping
  - ✅ Structured diagnostics with severity levels and source spans
  - ❌ Stringification `#` operator (not implemented, deferred)
  - ❌ Token pasting `##` operator (not implemented, deferred)
  - 🟡 Proper rescanning with paint semantics (basic recursion prevention only)
- Unit tests
  - Preprocessor: basic, macro expansion, conditionals, includes, constant expressions
  - Tokenizer: whitespace, punctuators, strings, chars, pp-number, identifiers
  - Updated to use range-style iteration over streaming tokenizer
  - 13 test suites all passing

## In Progress / Near-Term
- **Parser for translation units (next major milestone)**
  - Type system foundation (scalar types, pointers, arrays, structs, unions, enums)
  - Expression parser with operator precedence
  - Statement parser (compound, if, while, for, do-while, switch, return, break, continue)
  - Declaration parser with declarators
  - Function definitions
  - AST construction with source spans
- Semantic analysis preparation
  - Symbol tables and scope management
  - Type checking and conversions
  - Constant expression evaluation (semantic, not preprocessor)
  - Lvalue/rvalue analysis
- Preprocessor enhancements (deferred)
  - Stringification `#` operator in macro replacements
  - Token pasting `##` operator
  - `#error`, `#warning`, `#line`, `#pragma` directives
  - Predefined macros (`__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`)
  - Include guard optimization

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

## Preprocessor Phases Summary (C17 §5.1.1.2)
- Phase 1–3: ✅ Implemented (trigraphs, line splicing, comment removal, tokenization)
- Phase 4: ✅ Implemented (directives, macro expansion, conditionals, includes)
- Phase 5–6: 🚧 Deferred (escape sequence interpretation, string concatenation handled in parser)
- Phase 7: 📋 Next (token conversion from preprocessing-tokens to language tokens; parser integration)
- Phase 8: 📋 Future (linkage and translation unit merging)

## Notes
- Freestanding target: no libc/WASI; emit Wasm modules directly.
- Unit tests should evolve alongside features; prefer focused tests with clear expectations and token dumps on failure.
