# Preprocessor Directives Design (Updated)

This document outlines the design and phased plan to implement a C preprocessor for WVMCC. Tokenization (including phase 1–3 preprocessing) is implemented via a streaming `Tokenizer` and `SourceBuffer`. The Tokenizer now provides `next()`, `peek()`, and range-style iteration; the former batch `tokenize()` API has been removed. This plan focuses on directive parsing, macro expansion, conditional compilation, and includes, moving toward a single-pass model.

## Goals
- Standards-compliant handling of C preprocessor directives (target C11/C17 semantics)
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

### Phase 1: Directive Parsing (foundation) — Done
- Detect `#` at start of logical line (after optional whitespace)
- Parse directive keyword and capture trailing tokens until newline
- Validate syntax and produce `Directive` objects
- Status: Parsing implemented and integrated; tests updated to use streaming Tokenizer.

### Phase 2: Object-like Macros
- Implement `#define NAME value` and `#undef NAME`
- Expand macros in token stream with rescanning; prevent infinite recursion
- Respect identifier boundaries; no expansion inside string/char literals

### Phase 3: Function-like Macros
- Implement `#define NAME(arg1, ...) replacement`
- Argument substitution, stringification `#`, token pasting `##`, variadic handling `__VA_ARGS__`
- Blue-paint marking to avoid re-expansion within same pass

### Phase 4: Conditional Compilation
- Implement `#if` expression evaluation (integer constant expressions)
- Implement `defined(NAME)` operator within `#if`
- Support `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif` with nesting
- Skip inactive regions while still recognizing matching `#endif`
- Status: **Not implemented yet**; Example 2 include test is defined but not executed until this lands.

### Phase 5: Includes
- Implement `#include <...>` (system) and `#include "..."` (local)
- Configurable include search paths
- Detect and honor include guards; prevent cycles
- Current status:
    - Quote includes search the current file directory then fall back to angle search paths (`-I`), reusing the same header-name sequence (C11 6.10.2).
    - Macro-replaced `#include` is supported: tokens after `include` are macro-expanded and reinterpreted as `<...>` or `"..."`; diagnostics report missing/unterminated header-names.
    - Cycle detection via inclusion stack is in place; nesting-depth enforcement is deferred.

### Phase 6: Utilities
- Implement `#error` (emit failure) and `#warning` (optional)
- Implement `#line` (affect emitted `SourcePos`)
- Handle `#pragma` as pass-through or targeted behaviors

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
- `#pragma once` fast-path support via include handler
- Configurable predefined macros by target/flags
- File-level preprocessing API (beyond path-based run)

