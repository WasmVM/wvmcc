# Preprocessor Directives Design (Updated)

This document outlines the design and phased plan to implement a C preprocessor for WVMCC. Tokenization (including phase 1–3 preprocessing) is implemented via a streaming `Tokenizer` and `SourceBuffer`. The Tokenizer now provides `next()`, `peek()`, and range-style iteration; the former batch `tokenize()` API has been removed. This plan focuses on directive parsing, macro expansion, conditional compilation, and includes, moving toward a single-pass model.

## Goals
- Standards-compliant handling of C17 preprocessor directives (ISO/IEC 9899:2018)
- Efficient single-pass behavior with clear separation of responsibilities
- Robust error reporting with source positions
- Deterministic macro expansion with recursion protection

## Components

- DirectiveParser: Parses lines beginning with `#` into directive structures.
- MacroTable: Stores and manages object-like and function-like macros.
- ConditionalState: Tracks nested conditional compilation blocks.
- IncludeHandler: Resolves `#include` directives and manages include depth/guards.
- Preprocessor: Orchestrates streamed token processing, directive execution, and macro expansion.

## Tokenizer API (Streaming)

```
Tokenizer tz(input);
// Stream tokens
while (auto t = tz.next()) { /* use *t */ }

// Lookahead without consuming
if (auto p = tz.peek()) { /* inspect */ }

// Range-style iteration
for (const auto& tok : tz) { /* use tok */ }
```

Notes:
- `SourceBuffer` performs phases 1–3 (trigraphs, EOL normalization, line splicing, comment removal).
- Punctuator recognition uses a switch-based longest-match dispatcher.
- Helpers like `is_hex` and `is_oct` live on `Tokenizer`.
- `reset()` reinitializes stream state; `peek()` caches one lookahead token.

## Directive Set
- Macro definition: `#define`, `#undef`
- Includes: `#include <...>`, `#include "..."`
- Conditionals: `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`
- Utilities: `#error`, `#warning` (optional), `#pragma`, `#line`
- Predefined macros: `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`, `__STDC__`, `__STDC_VERSION__`

## Data Structures

```cpp
enum class DirectiveKind { Define, Undef, Include, If, Ifdef, Ifndef, Else, Elif, Endif, Error, Warning, Pragma, Line };

struct Directive {
    DirectiveKind kind;
    SourcePos pos;
    std::vector<PPToken> args; // tokens after directive keyword on the line
};

class MacroTable {
public:
    // add/remove/query macros (object-like and function-like)
};

class ConditionalState {
public:
    // push/pop conditional frames, evaluate active state
};

class IncludeHandler {
public:
    // resolve paths, track include depth, handle guards
};

class Preprocessor {
public:
    PreprocessResult run(const std::string& inputPath);
private:
    MacroTable macros;
    ConditionalState cond;
    IncludeHandler includes;

    std::optional<Directive> parseDirective(size_t& i, const std::vector<PPToken>& toks);
    std::vector<PPToken> processDirectives(std::vector<PPToken> toks);
    std::vector<PPToken> expandMacros(const std::vector<PPToken>& toks);
};
```

## Phased Plan

### Phase 1: Directive Parsing (foundation) — ✅ Done
- Detect `#` at start of logical line (after optional whitespace)
- Parse directive keyword and capture trailing tokens until newline
- Validate syntax and produce `Directive` objects
- Status: Parsing implemented and integrated; tests updated to use streaming Tokenizer.

### Phase 2: Object-like Macros — ✅ Done
- Implement `#define NAME value` and `#undef NAME`
- Expand macros in token stream with rescanning; prevent infinite recursion
- Respect identifier boundaries; no expansion inside string/char literals
- Status: Object-like macros fully implemented with expansion tracking to prevent recursion.

### Phase 3: Function-like Macros — ✅ Done
- Implement `#define NAME(arg1, ...) replacement`
- ✅ Argument substitution and variadic handling `__VA_ARGS__`
- ✅ Stringification `#` operator (converts parameter to string literal)
- ✅ Token pasting `##` operator (concatenates adjacent tokens)
- ✅ **Full paint semantics and proper rescanning implemented**
- **Current Status**: Full paint semantics per C17 §6.10.3.3 implemented:
  - Each token tracks painted macros via `paintedMacros` set in `PPToken`
  - Tokens from macro replacement are marked as painted with that macro name
  - Painted tokens skip expansion when the same macro is encountered again
  - Recursive expansion works correctly for multi-level macro chains
  - Handles complex cases including nested stringification and token pasting

### Phase 4: Conditional Compilation — ✅ Done
- Implement `#if` expression evaluation per C17 6.6 (integer constant expressions)
  - Support integer/character constants, defined(NAME) preprocessor operator
  - Arithmetic, bitwise, logical, relational, equality, and ternary operators
  - Reject disallowed operators: assignment, increment/decrement, function calls, comma
  - Division and modulo by zero emit diagnostics
  - Macro expansion before expression parse (defined operands protected from expansion)
- Implement `defined(NAME)` operator: evaluates to 1 if NAME is defined, 0 otherwise
- Support `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif` with arbitrary nesting
- Skip tokens in inactive regions; continue parsing directives to maintain nesting balance
- Emit diagnostics for unmatched/duplicate directives and malformed expressions
- Status: Fully implemented with ConstExprParser; comprehensive test coverage including error cases.
- Note: sizeof not yet supported in constant expressions.

### Phase 5: Includes — ✅ Done
- Implement `#include <...>` (system) and `#include "..."` (local)
- Configurable include search paths
- Detect and honor include guards; prevent cycles
- Status: Fully implemented.
    - Quote includes search the current file directory first, then fall back to angle-style search paths (`-I`), reusing the same header-name sequence (C17 6.10.2).
    - Macro-replaced `#include` is supported: tokens after `include` are macro-expanded and reinterpreted as `<...>` or `"..."`; diagnostics report missing/unterminated header-names.
    - Cycle detection via inclusion stack is in place; comprehensive test coverage.
    - Include guards optimization deferred.

### Phase 6: Utilities — 🚧 Deferred
- Implement `#error` (emit failure) and `#warning` (optional)
- Implement `#line` (affect emitted `SourcePos`)
- Handle `#pragma` as pass-through or targeted behaviors
- Status: Not yet implemented; planned for later milestone.

### Phase 7 (Enhancement): Macro Replacement Operators — ✅ Done
- Implement stringification `#` operator (C17 §6.10.3.1) — ✅ Complete
  - Convert macro parameter to string literal
  - Escape handling: insert `\` before each `"` and `\` in literals
  - Whitespace normalization between tokens
- Implement token pasting `##` operator (C17 §6.10.3.2) — ✅ Complete
  - Concatenate adjacent tokens into single token
  - Placemarker handling for empty arguments
  - Token validation after concatenation
  - Rescanning for further macro replacement
- Implement full rescanning with paint semantics (C17 §6.10.3.3) — ✅ Complete
  - Track expansion state with painted tokens (not just per-invocation set)
  - Painted names skip expansion in all nested replacements
  - Handle complex cases like the `hash_hash` example in §6.10.3.2
- Status: All macro replacement operators and paint semantics fully implemented and tested.

## Implementation Status Summary

| Feature | Status | Notes |
|---------|--------|-------|
| Object-like macros | ✅ | Full support with expansion |
| Function-like macros | ✅ | Parameters, argument substitution |
| Variadic macros `__VA_ARGS__` | ✅ | Per C17 §6.10.3.1 |
| Stringification `#` | ✅ | Fully implemented with escape handling |
| Token pasting `##` | ✅ | Fully implemented with multi-paste support |
| Full rescanning with paint | ✅ | Per C17 §6.10.3.3, paint tracking in PPToken |
| `#define` / `#undef` | ✅ | Full support |
| Conditional directives | ✅ | All variants |
| Constant expressions | ✅ | C17 §6.6 (except `sizeof`) |
| `#include` directives | ✅ | Paths, macros, cycles |
| Predefined macros | ✅ | `__FILE__`, `__DATE__`, `__TIME__`, `__STDC__`, `__STDC_VERSION__`, `__STDC_HOSTED__`, `__STDC_NO_ATOMICS__`, `__STDC_NO_COMPLEX__`, `__STDC_NO_THREADS__` |
| `#error`, `#warning` | ❌ | Phase 6, deferred |
| `#line`, `#pragma` | ❌ | Phase 6, deferred |

## Error Handling
- All directive errors must include `SourcePos` (line, column)
- Examples: unterminated directive, bad macro parameters, unmatched conditionals

## Testing Strategy
- Unit tests per directive type
- Macro expansion edge cases (nested, recursive, paste/stringification)
- Conditional nesting and inactive region skipping
- Include path resolution + guard detection
- Negative tests for error reporting

Tokenizer-focused tests use range-style iteration to verify phases 1–3 and literal handling.

## Performance Considerations
- Minimize rescans; use efficient token views
- Cache include file contents
- Avoid quadratic behavior in macro substitution by linear passes with markers

## Future Extensions
- `__LINE__` macro with dynamic line tracking (currently not supported)
- `#pragma once` fast-path support via include handler
- Diagnostic directives: `#error`, `#warning`
- Line information: `#line` directive
- Utilities: `#pragma` extended handling
- Configurable predefined macros by target/flags
- File-level preprocessing API (beyond path-based run)
- Include guard optimization

