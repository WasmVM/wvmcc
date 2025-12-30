# Project Milestones

This document tracks key milestones achieved so far and the upcoming work items for the freestanding C17 → Wasm compiler.

## Completed
- Repository scaffolding with CMake, tests, and CLI entrypoints
# Project Milestones

This document tracks key milestones and the current implementation state for the freestanding C17 → Wasm compiler.

## Completed

- Repository scaffold and build/tests: CMake-based build and unit-test integration (top-level `CMakeLists.txt`, `build-Debug/`, `Testing/`).

- Tokenizer (streaming)
  - Streaming API: `next()`, `peek()`, `reset()`, iterator/range-style iteration (`src/pp/Tokenizer.hpp`, `src/pp/Tokenizer.cpp`).
  - Implements phases 1–3 behaviors: trigraph handling, EOL normalization, line splicing, comment handling, explicit `whitespace`/`newline` tokens.
  - Full support for punctuators (longest-match), header-name detection for `#include`, string literals, char constants, identifiers (UCNs), and pp-number rules.

- Preprocessor (phases 4–7 implemented)
  - Directive parsing and execution: `#define/#undef/#include/#if/#ifdef/#ifndef/#elif/#else/#endif`, `#error`, `#warning`, `#line`, `#pragma` (`src/pp/Preprocessor.hpp`, `src/pp/Preprocessor.cpp`).
  - Macro system: object-like and function-like macros, variadic macros (`__VA_ARGS__`), argument substitution, recursion protection (`src/pp/MacroTable.hpp`/`.cpp`).
  - Replacement operators: stringification (`#`), token pasting (`##`), placemarkers and rescanning implemented.
  - Paint semantics / full rescanning per C17 §6.10.3.3 implemented (paint tracking in `PPToken`).
  - Conditional compilation with `ConstExprParser` for `#if` evaluation; `defined()` operator semantics respected (`src/pp/ConstExprParser.hpp`/`.cpp`).
  - `#include` handling: quote/angle includes, macro-expanded includes, cycle detection, and search path resolution.
  - Predefined macros and diagnostics implemented (`src/pp/Diagnostics.hpp`).

- Parser & AST (M0 subset): ✅ Implemented
  - Recursive-descent `Parser` with declarator grammar, struct/union/enum specifiers, declarators, initializers, function definitions, and core statement forms (`src/parser/Parser.cpp`, `src/parser/AST.hpp`, `src/parser/Parser.hpp`).
  - Expression parser with precedence, conditional expressions, and constant-expression checks using `ConstExprEvaluator` (`src/parser/ConstExprEval.*`).
  - Semantic scaffolding: typedef/tag registries, internal-definition tracking, labels/goto tracking, and diagnostic emission (`src/parser/Semantic.*`).

- Tests
  - Unit tests for tokenizer, preprocessor, and parser (test executables under `Testing/tests/`), with multiple suites exercising macro replacement, conditionals, includes, and parser features.

## In Progress / Near-Term

- Semantic analysis
  - Complete type checking, conversions, lvalue/rvalue analysis, and integration of semantic constant-expression evaluation for initializers and compile-time checks.
  - Improve diagnostics, recovery, and cross-file symbol handling (linkage/definitions).

- IR & Codegen
  - Design of a simple IR for lowering expressions/statements to Wasm-like operations.
  - Backend lowering to `WasmModule` via `WasmVM` integration (module encoding and emission).

- Preprocessor enhancements
  - Include-guard pattern detection/optimization and configurable predefined macros per target/flags.
  - Additional robustness tests for complex macro chains and pathological rescanning cases.

## Later Milestones

- Complete C17 feature coverage (progressively beyond M0): aggregate initialization, bitfields layout, variadic ABI refinements, `_Generic`, `_Static_assert`, and other C17 items deferred earlier.
- Full IR optimizations and validation passes, additional backend improvements, and optional runtime stubs for freestanding execution.

## Preprocessor Phases Summary (C17 §5.1.1.2)

- Phase 1–3: ✅ Implemented (trigraphs, EOL normalization, line splicing, comment handling, tokenization).
- Phase 4: ✅ Implemented (directive parsing/execution, includes, macro expansion starter behaviors).
- Phase 5–7: ✅ Implemented (character/escape handling, string concatenation, macro replacement operators, token pasting, stringification, and paint semantics/rescanning).
- Phase 8 (linkage/translation-unit merging): 📋 Planned (work tied to semantic/linking and codegen milestones).

## Notes

- Code references: core implementations live under `src/pp/` (preprocessor/tokenizer), `src/parser/` (parser/AST/semantic), and `src/exec/` (CLI/driver).
- Build and tests: use CMake out-of-source build; test binaries live in `Testing/tests/` and `build-Debug/Testing` when configured for Debug.
