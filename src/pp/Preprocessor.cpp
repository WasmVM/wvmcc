#include "Preprocessor.hpp"
#include "ConstExprParser.hpp"

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <cctype>
#include "Tokenizer.hpp"

namespace wvmcc {

// Forward declarations for helpers used by methods defined earlier in this file
static std::string extractLiteralPrefix(const std::string &lex);
static bool extractLiteralInnerAndQuote(const std::string &lex, std::string &outInner, char &outQuote);

// Helper function to stringify tokens (for # operator in macros)
// Now a member method (forward reference removed)

// (Removed unused helper `file_ends_with_newline`)

// Minimal streaming API implementation (object-like macros, includes, simple conditionals)

bool Preprocessor::open(const std::string& inputPath) {
    return pushFile(inputPath);
}

std::optional<PPToken> Preprocessor::peek() {
    ensureBuffer();
    if (outBuffer.empty()) return std::nullopt;
    // Return a normalized copy so callers always see decoded literals
    PPToken copy = outBuffer.front();
    normalizeLiteralToken(copy);
    return copy;
}

std::optional<PPToken> Preprocessor::next() {
    ensureBuffer();
    if (outBuffer.empty()) return std::nullopt;
    auto t = outBuffer.front(); outBuffer.pop_front();

    // If this is a string literal, perform Phase 6 adjacent literal concatenation
    if (t.kind == PPTokenKind::StringLiteral) {
        // Normalize left component
        normalizeLiteralToken(t);
        // Ensure we have buffered following tokens (read-ahead) before attempting to merge
        ensureBuffer();
        // Consume and merge following whitespace/newline + string-literal sequences
        while (!outBuffer.empty()) {
            // Skip whitespace/newline between adjacent literals
            while (!outBuffer.empty() && (outBuffer.front().kind == PPTokenKind::Whitespace || outBuffer.front().kind == PPTokenKind::Newline)) {
                outBuffer.pop_front();
            }
            if (outBuffer.empty()) break;
            if (outBuffer.front().kind != PPTokenKind::StringLiteral) break;

            PPToken right = outBuffer.front(); outBuffer.pop_front();
            normalizeLiteralToken(right);

            // Extract inners and prefixes
            std::string lpre, rpre, lin, rin; char lq=0, rq=0;
            lpre = extractLiteralPrefix(t.lexeme);
            rpre = extractLiteralPrefix(right.lexeme);
            if (!extractLiteralInnerAndQuote(t.lexeme, lin, lq) || !extractLiteralInnerAndQuote(right.lexeme, rin, rq)) {
                // malformed: stop merging
                break;
            }
            if (lq != rq) {
                diagnostics.push_back(Diagnostic{.message = "incompatible string literal quote types during concatenation", .severity = Diagnostic::Severity::Error, .span = std::nullopt});
                break;
            }

            std::string combinedPrefix;
            if (lpre.empty() && rpre.empty()) combinedPrefix = std::string();
            else if (lpre.empty()) combinedPrefix = rpre;
            else if (rpre.empty()) combinedPrefix = lpre;
            else if (lpre == rpre) combinedPrefix = lpre;
            else {
                diagnostics.push_back(Diagnostic{.message = "incompatible string literal prefixes during concatenation", .severity = Diagnostic::Severity::Error, .span = std::nullopt});
                // Do not merge; put right back at front and stop
                outBuffer.push_front(right);
                break;
            }

            // Merge inners
            std::string newInner = lin + rin;
            std::string rebuilt = combinedPrefix + std::string(1, lq) + newInner + std::string(1, lq);
            t.lexeme = rebuilt;
            t.span.end = right.span.end;
            t.copyPaint(right);
        }
        return t;
    }

    normalizeLiteralToken(t);
    return t;
}

void Preprocessor::reset() {
    outBuffer.clear();
    fileStack.clear();
    macroTable.clear();
    atLineStart = true;
}

bool Preprocessor::empty() {
    ensureBuffer();
    return outBuffer.empty();
}

bool Preprocessor::pushFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    auto ssp = std::make_unique<std::istringstream>(ss.str());
    auto tz = std::make_unique<Tokenizer>(*ssp);
    FileCtx ctx;
    ctx.path = path;
    ctx.stream = std::move(ssp);
    ctx.tokenizer = std::move(tz);
    // Initialize tokenizer state for streaming use
    ctx.tokenizer->reset();
    ctx.dir = std::filesystem::path(path).parent_path().string();
    // Track inclusion stack for streaming mode and define predefined macros
    namespace fs = std::filesystem;
    std::string canonicalPath = fs::weakly_canonical(fs::absolute(path)).string();
    if (!pushInclusion(canonicalPath)) {
        return false; // cycle detected
    }
    (void)canonicalPath;
    fileStack.push_back(std::move(ctx));
    atLineStart = true;

    // If this is the first file in the inclusion stack, initialize predefined macros
    if (inclusionStack.size() == 1) {
        // __FILE__
        std::vector<PPToken> replacement;
        replacement.push_back(PPToken{ .kind = PPTokenKind::StringLiteral, .span = {}, .lexeme = "\"" + canonicalPath + "\"", .paintedMacros = {} });
        macroTable.defineObjectMacro("__FILE__", replacement);

        // __DATE__
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        char dateStr[32];
        std::strftime(dateStr, sizeof(dateStr), "%b %d %Y", &tm);
        std::vector<PPToken> dateRepl;
        dateRepl.push_back(PPToken{ .kind = PPTokenKind::StringLiteral, .span = {}, .lexeme = std::string("\"") + dateStr + "\"", .paintedMacros = {} });
        macroTable.defineObjectMacro("__DATE__", dateRepl);

        // __TIME__
        char timeStr[32];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
        std::vector<PPToken> timeRepl;
        timeRepl.push_back(PPToken{ .kind = PPTokenKind::StringLiteral, .span = {}, .lexeme = std::string("\"") + timeStr + "\"", .paintedMacros = {} });
        macroTable.defineObjectMacro("__TIME__", timeRepl);

        // __STDC__ and related
        std::vector<PPToken> p1; p1.push_back(PPToken{ .kind = PPTokenKind::PPNumber, .span = {}, .lexeme = "1", .paintedMacros = {} });
        macroTable.defineObjectMacro("__STDC__", p1);
        std::vector<PPToken> ver; ver.push_back(PPToken{ .kind = PPTokenKind::PPNumber, .span = {}, .lexeme = "201710L", .paintedMacros = {} });
        macroTable.defineObjectMacro("__STDC_VERSION__", ver);
        std::vector<PPToken> zero; zero.push_back(PPToken{ .kind = PPTokenKind::PPNumber, .span = {}, .lexeme = "0", .paintedMacros = {} });
        macroTable.defineObjectMacro("__STDC_HOSTED__", zero);
        std::vector<PPToken> one; one.push_back(PPToken{ .kind = PPTokenKind::PPNumber, .span = {}, .lexeme = "1", .paintedMacros = {} });
        macroTable.defineObjectMacro("__STDC_NO_ATOMICS__", one);
        macroTable.defineObjectMacro("__STDC_NO_COMPLEX__", one);
        macroTable.defineObjectMacro("__STDC_NO_THREADS__", one);
    }

    return true;
}

std::optional<PPToken> Preprocessor::readRawToken() {
    while (!fileStack.empty()) {
        auto &ctx = fileStack.back();
    
        auto tok = ctx.tokenizer->next();
        if (!tok) {
            // End of current file: pop file context and corresponding inclusion stack entry
            fileStack.pop_back();
            popInclusion();
            // After returning from an included file, we are at the start of the next line
            atLineStart = true;
            continue;
        }
        
        (void)ctx; (void)tok;
        return tok;
    }
    return std::nullopt;
}

void Preprocessor::ensureBuffer() {
    // Keep reading tokens until we have at least one token to return.
    // Additionally, if the last token we produced is a string literal,
    // continue reading to allow adjacent string-literal concatenation
    // to occur before we return any token.
    while (outBuffer.empty() || (!outBuffer.empty() && outBuffer.back().kind == PPTokenKind::StringLiteral)) {
        auto opt = readRawToken();
        if (!opt) return;
        auto tok = *opt;

        // Newline handling
        if (handleNewlineToken(tok)) continue;

        // Inside inactive conditional block: skip unless directive/newline
        if (!conditionalStack.empty() && !conditionalStack.back().currentlyActive) {
            if (handleInactiveConditionalToken(tok)) continue;
        }

        // Line-start directive
        if (atLineStart && tok.kind == PPTokenKind::Punctuator && tok.lexeme == "#") {
            handleDirective();
            continue;
        }

        // Identifier/macro handling
        if (tok.kind == PPTokenKind::Identifier) {
            if (macroTable.isDefined(tok.lexeme)) {
                if (handleIdentifierExpansionToken(tok)) continue;
            }
        }

        // Default emit (including __LINE__ dynamic expansion)
        emitTokenOrLineExpansion(tok);
    }
}

bool Preprocessor::handleNewlineToken(const PPToken& tok) {
    if (tok.kind == PPTokenKind::Newline) {
        outBuffer.push_back(tok);
        atLineStart = true;
        return true;
    }
    return false;
}

bool Preprocessor::handleInactiveConditionalToken(const PPToken& tok) {
    if (tok.kind == PPTokenKind::Newline) {
        outBuffer.push_back(tok);
        atLineStart = true;
        return true;
    }
    if (atLineStart && tok.kind == PPTokenKind::Punctuator && tok.lexeme == "#") {
        handleDirective();
        return true;
    }
    // Otherwise skip silently
    return true;
}

bool Preprocessor::handleIdentifierExpansionToken(const PPToken& tok) {
    auto mOpt = macroTable.getMacro(tok.lexeme);
    if (!mOpt) return false;
    const Macro* m = *mOpt;
    if (!m->isFunction) {
        std::vector<PPToken> substituted;
        for (auto repl : m->replacement) {
            repl.paint(m->name);
            if (repl.kind == PPTokenKind::Identifier && repl.lexeme == "__LINE__") {
                repl.span = tok.span;
            }
            substituted.push_back(repl);
        }
        auto expanded = expandMacros(substituted);
        for (auto it = expanded.rbegin(); it != expanded.rend(); ++it) {
            pushTokenFrontWithConcat(*it);
        }
        return true;
    } else {
        return tryHandleFunctionLikeMacroInvocation(m, tok);
    }
    return false;
}

bool Preprocessor::tryHandleFunctionLikeMacroInvocation(const Macro* m, const PPToken& tok) {
    if (fileStack.empty()) return false;
    if (!consumeOptionalWhitespaceBeforeOpenParen()) return false;
    // Collect invocation tokens and attempt expansion
    std::vector<PPToken> invocation;
    invocation.push_back(tok);
    auto rest = collectInvocationTokens();
    for (auto &pt : rest) invocation.push_back(pt);
    size_t idx = 0;
    std::vector<PPToken> expandedResult;
    bool saved = batchPushNoLookahead;
    batchPushNoLookahead = true;
    if (tryExpandFunctionLikeMacro(invocation, idx, m, tok, expandedResult)) {
        for (const auto &et : expandedResult) pushTokenBackWithConcat(et);
    } else {
        for (const auto &itok : invocation) pushTokenBackWithConcat(itok);
    }
    batchPushNoLookahead = saved;
    return true;
}

bool Preprocessor::consumeOptionalWhitespaceBeforeOpenParen() {
    if (fileStack.empty()) return false;
    auto &ctx = *fileStack.back().tokenizer;
    while (auto p = ctx.peek()) {
        if (p->kind == PPTokenKind::Whitespace) { ctx.next(); continue; }
        break;
    }
    auto nextTok = ctx.peek();
    return nextTok && nextTok->kind == PPTokenKind::Punctuator && nextTok->lexeme == "(";
}

void Preprocessor::emitTokenOrLineExpansion(const PPToken& tok) {
    if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "__LINE__") {
    std::string lineStr = std::to_string(tok.span.begin.line);
        PPToken numTok{
            .kind = PPTokenKind::PPNumber,
            .span = tok.span,
            .lexeme = lineStr,
            .paintedMacros = {}
        };
        pushTokenBackWithConcat(numTok);
    } else {
        pushTokenBackWithConcat(tok);
    }
    atLineStart = false;
}

// Note: directive routing/refactor helpers removed; `handleDirective` remains the active directive parser.

void Preprocessor::skipLineFromToken(PPToken t) {
    if (t.kind == PPTokenKind::Newline) return;
    std::optional<PPToken> tn;
    while ((tn = readRawToken())) { if (tn->kind == PPTokenKind::Newline) break; }
}

void Preprocessor::handleDirective() {
    auto nameOpt = readDirectiveName();
    if (!nameOpt.has_value()) return;
    std::string dir = *nameOpt;

    // Fast-path wrappers for simple directives handled by streaming layer
    if (handleSimpleDirective(dir)) return;

    // Compound directives that require parsing the rest of the line
    if (dir == "if" || dir == "elif") {
        if (handleIfOrElifDirective(dir)) return;
        return;
    }

    if (dir == "else") {
        skipRestOfDirectiveTokens();
        handleElseDirective();
        return;
    }

    if (dir == "error" || dir == "warning" || dir == "line" || dir == "pragma") {
        if (handleUtilityDirective(dir)) return;
        return;
    }

    if (dir == "endif") { handleEndifDirective(); return; }

    // Unknown directive: consume rest of line
    skipRestOfDirectiveTokens();
}

std::optional<std::string> Preprocessor::readDirectiveName() {
    std::optional<PPToken> tok = readRawToken();
    if (!tok) return std::nullopt;
    while (tok && tok->kind == PPTokenKind::Whitespace) tok = readRawToken();
    if (!tok) return std::nullopt;
    if (tok->kind != PPTokenKind::Identifier) {
        if (tok) skipLineFromToken(std::move(*tok));
        return std::nullopt;
    }
    return tok->lexeme;
}

bool Preprocessor::handleSimpleDirective(const std::string& dir) {
    if (dir == "define") { handleDefine(); return true; }
    if (dir == "undef") { handleUndef(); return true; }
    if (dir == "include") { handleInclude(); return true; }
    if (dir == "ifdef") { handleIfdef(true); return true; }
    if (dir == "ifndef") { handleIfdef(false); return true; }
    return false;
}

std::vector<PPToken> Preprocessor::collectInvocationTokens() {
    std::vector<PPToken> invocation;
    int parenDepth = 0;
    while (true) {
        auto nt = readRawToken();
        if (!nt) break;
        invocation.push_back(*nt);
        if (nt->kind == PPTokenKind::Punctuator) {
            if (nt->lexeme == "(") parenDepth++;
            else if (nt->lexeme == ")") {
                parenDepth--;
                if (parenDepth <= 0) break;
            }
        }
    }
    return invocation;
}

bool Preprocessor::handleIfOrElifDirective(const std::string& dir) {
    std::vector<PPToken> lineTokens;
    while (true) {
        auto t = readRawToken();
        if (!t || t->kind == PPTokenKind::Newline) break;
        lineTokens.push_back(*t);
    }
    if (fileStack.empty()) return false;
    if (dir == "if") {
        return handleIfDirective(*fileStack.back().tokenizer, lineTokens);
    } else { // elif
        return handleElifDirective(*fileStack.back().tokenizer, lineTokens);
    }
}

bool Preprocessor::handleUtilityDirective(const std::string& dir) {
    bool active = checkActiveState();
    std::vector<PPToken> lineTokens;
    while (true) {
        auto t = readRawToken();
        if (!t || t->kind == PPTokenKind::Newline) break;
        lineTokens.push_back(*t);
    }
    if (!active) return true;
    if (dir == "error") { handleErrorDirective(lineTokens); }
    else if (dir == "warning") { handleWarningDirective(lineTokens); }
    else if (dir == "line") { handleLineDirective(lineTokens); }
    else if (dir == "pragma") { handlePragmaDirective(lineTokens); }
    return true;
}

void Preprocessor::skipRestOfDirectiveTokens() {
    std::optional<PPToken> t;
    while ((t = readRawToken())) { if (t->kind == PPTokenKind::Newline) break; }
}

void Preprocessor::handleDefine() {
    if (fileStack.empty()) return;
    auto &tz = *fileStack.back().tokenizer;
    (void)handleDefineDirective(tz);
}

void Preprocessor::handleUndef() {
    if (fileStack.empty()) return;
    auto &tz = *fileStack.back().tokenizer;
    (void)handleUndefDirective(tz);
}

void Preprocessor::handleInclude() {
    if (fileStack.empty()) return;
    auto &tz = *fileStack.back().tokenizer;
    std::vector<PPToken> temp;
    std::string currentDir = fileStack.back().dir;
    (void)handleIncludeDirective(tz, temp, currentDir);
    // Suppress tokenizer lookahead inside pushTokenBackWithConcat: the
    // tokenizer points at source AFTER the include directive, so any
    // lookahead would splice post-include tokens into the included header's
    // string literals (regression caught by pp-sysroot-include's adjacent
    // literal cases until this guard was added).
    bool saved = batchPushNoLookahead;
    batchPushNoLookahead = true;
    for (const auto &tk : temp) pushTokenBackWithConcat(tk);
    batchPushNoLookahead = saved;
}

void Preprocessor::handleIfdef(bool wantDefined) {
    auto tok = readRawToken();
    while (tok && tok->kind == PPTokenKind::Whitespace) tok = readRawToken();
    bool take = false;
    if (tok && tok->kind == PPTokenKind::Identifier) {
        bool def = macroTable.isDefined(tok->lexeme);
        take = wantDefined ? def : !def;
    }
    // Push a conditional frame, exactly like #if (handleIfDirective): the
    // streaming loop skips tokens while the top frame is inactive, #else/#elif
    // flip it, and the matching #endif pops it. The previous implementation
    // eager-skipped only the not-taken case and pushed NOTHING when taken, so
    // a taken #ifdef/#ifndef (e.g. an include guard's first pass) left its
    // #endif unbalanced — reported as a spurious "#endif without matching #if".
    // currentlyActive ANDs in the parent's state (checkActiveState over the
    // frames pushed so far) so an #ifdef nested in an inactive block stays
    // inactive even when its own condition is true.
    ConditionalFrame frame;
    frame.seenTrueBranch = take;
    frame.currentlyActive = take && checkActiveState();
    frame.inElse = false;
    conditionalStack.push_back(frame);
}

bool Preprocessor::checkActiveState() {
    for (const auto &f : conditionalStack) {
        if (!f.currentlyActive) return false;
    }
    return true;
}

bool Preprocessor::checkParentActiveState() {
    // Active state of every frame except the current top frame. #elif/#else
    // operate on the already-pushed top frame, so they must AND in the parent
    // chain (not the whole stack) when re-activating a branch — otherwise a
    // true #elif/#else nested in an inactive block would wrongly emit output.
    if (conditionalStack.empty()) return true;
    for (size_t i = 0; i + 1 < conditionalStack.size(); ++i) {
        if (!conditionalStack[i].currentlyActive) return false;
    }
    return true;
}


// Removed unused directive-routing helpers (processDirective, routeDirective, skipDirectiveWhitespace,
// consumeUnknownDirective, handleDirectiveInclude) and literal/emit helpers (validateLiteralToken, emitIfActive).

// `processLineStartToken` removed — unused after switching to streaming `ensureBuffer`.

bool Preprocessor::executeInclude(const std::string& header, bool isAngle,
                                   std::optional<SourceSpan> span,
                                   std::vector<PPToken>& out,
                                   const std::string& currentDir) {
    if (auto resolved = resolveInclude(header, isAngle, currentDir)) {
        if (pragmaOnceFiles.count(*resolved) > 0) {
            return true; // Already included with #pragma once
        }
        
        Preprocessor child;
        child.includePaths = includePaths;
        child.systemIncludePaths = systemIncludePaths;
        child.sysroot = sysroot;
        child.inclusionStack = inclusionStack;
        child.pragmaOnceFiles = pragmaOnceFiles;
        // Inherit the parent's macros so the included file can see them
        // (matches C preprocessor semantics — feature-test macros are
        // typically defined before #include lines).
        for (const auto& [name, m] : macroTable) {
            child.macroTable.insertOrAssign(name, m);
        }
        // Use streaming API on child to collect tokens from included file
        if (!child.open(*resolved)) {
            diagnostics.push_back(Diagnostic{
                .message = std::string("failed to open include file '") + *resolved + "'",
                .severity = Diagnostic::Severity::Error,
                .span = span
            });
            return false;
        }
        std::vector<PPToken> childTokens;
        while (auto t = child.next()) childTokens.push_back(*t);
        diagnostics.insert(diagnostics.end(), child.diagnostics.begin(), child.diagnostics.end());

        for (const auto& file : child.pragmaOnceFiles) {
            pragmaOnceFiles.insert(file);
        }
        // Propagate macros defined in the included file back to the
        // includer — this is the cross-include macro visibility that
        // standard C preprocessors give you.
        for (const auto& [name, m] : child.macroTable) {
            macroTable.insertOrAssign(name, m);
        }

        for (const auto& tk : childTokens) out.push_back(tk);
        return true;
    }
    diagnostics.push_back(Diagnostic{
        .message = std::string("include file not found: ") + header,
        .severity = Diagnostic::Severity::Error,
        .span = span
    });
    return false;
}

bool Preprocessor::processAngleBracketInclude(Tokenizer& tokenizer,
                                               std::vector<PPToken>& out,
                                               const std::string& currentDir) {
    auto tok = tokenizer.peek();
    if (!tok || tok->kind != PPTokenKind::Punctuator || tok->lexeme != "<") {
        return false;
    }
    
    tokenizer.next();
    std::string header;
    auto begin = tok->span.begin;
    SourcePos end = tok->span.end;
    while (auto t = tokenizer.peek()) {
        if (t->kind == PPTokenKind::Punctuator && t->lexeme == ">") {
            end = t->span.end;
            tokenizer.next();
            break;
        }
        if (t->kind == PPTokenKind::Newline) break;
        header += t->lexeme;
        tokenizer.next();
    }
    executeInclude(header, /*isAngle=*/true, SourceSpan{begin, end}, out, currentDir);
    return true;
}

bool Preprocessor::processStringLiteralInclude(Tokenizer& tokenizer,
                                                std::vector<PPToken>& out,
                                                const std::string& currentDir) {
    auto tok = tokenizer.peek();
    if (!tok || tok->kind != PPTokenKind::StringLiteral) {
        return false;
    }
    
    auto lit = *tokenizer.next();
    std::string header = lit.lexeme;
    if (header.size() >= 2 && header.front() == '"' && header.back() == '"') {
        header = header.substr(1, header.size() - 2);
    }
    if (executeInclude(header, /*isAngle=*/false, lit.span, out, currentDir)) return true;
    return executeInclude(header, /*isAngle=*/true, lit.span, out, currentDir);
}

bool Preprocessor::parseExpandedAngleBracket(const std::vector<PPToken>& expanded,
                                              size_t i,
                                              std::vector<PPToken>& out,
                                              const std::string& currentDir) {
    std::string header;
    size_t j = i + 1;
    int depth = 1;
    SourcePos begin = expanded[i].span.begin;
    SourcePos end = expanded[i].span.end;
    
    for (; j < expanded.size(); ++j) {
        if (expanded[j].kind == PPTokenKind::Punctuator && expanded[j].lexeme == "<") {
            depth++;
        } else if (expanded[j].kind == PPTokenKind::Punctuator && expanded[j].lexeme == ">") {
            if (--depth == 0) {
                end = expanded[j].span.end;
                break;
            }
        } else if (expanded[j].kind != PPTokenKind::Newline) {
            header += expanded[j].lexeme;
        }
    }
    
    if (depth == 0) {
        return executeInclude(header, /*isAngle=*/true, SourceSpan{begin, end}, out, currentDir);
    }
    
    diagnostics.push_back(Diagnostic{
        .message = std::string("unterminated header-name in macro-expanded #include"),
        .severity = Diagnostic::Severity::Error,
        .span = SourceSpan{begin, expanded.back().span.end}
    });
    return false;
}

bool Preprocessor::parseExpandedStringLiteral(const PPToken& lit,
                                               std::vector<PPToken>& out,
                                               const std::string& currentDir) {
    std::string header = lit.lexeme;
    if (header.size() >= 2 && header.front() == '"' && header.back() == '"') {
        header = header.substr(1, header.size() - 2);
    }
    if (executeInclude(header, /*isAngle=*/false, lit.span, out, currentDir)) return true;
    return executeInclude(header, /*isAngle=*/true, lit.span, out, currentDir);
}

bool Preprocessor::processMacroExpandedInclude(Tokenizer& tokenizer,
                                                std::vector<PPToken>& out,
                                                const std::string& currentDir) {
    auto tail = collectLineTokens(tokenizer);
    auto expanded = expandMacros(tail);
    size_t i = 0;
    while (i < expanded.size() && expanded[i].kind == PPTokenKind::Whitespace) i++;

    if (i >= expanded.size()) {
        diagnostics.push_back(Diagnostic{
            .message = std::string("invalid #include after macro expansion: missing header-name"),
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    if (expanded[i].kind == PPTokenKind::Punctuator && expanded[i].lexeme == "<") {
        return parseExpandedAngleBracket(expanded, i, out, currentDir);
    }

    if (expanded[i].kind == PPTokenKind::StringLiteral) {
        return parseExpandedStringLiteral(expanded[i], out, currentDir);
    }

    diagnostics.push_back(Diagnostic{
        .message = std::string("invalid #include after macro expansion: expected <header> or \"header\""),
        .severity = Diagnostic::Severity::Error,
        .span = expanded[i].span
    });
    return false;
}

bool Preprocessor::handleIncludeDirective(Tokenizer& tokenizer,
                                          std::vector<PPToken>& out,
                                          const std::string& currentDir) {
    // Skip spaces after include
    while (auto w = tokenizer.peek()) {
        if (w->kind == PPTokenKind::Whitespace) tokenizer.next();
        else break;
    }

    if (processAngleBracketInclude(tokenizer, out, currentDir)) return true;
    if (processStringLiteralInclude(tokenizer, out, currentDir)) return true;
    return processMacroExpandedInclude(tokenizer, out, currentDir);
}

std::optional<std::string> Preprocessor::resolveInclude(const std::string& header,
                                                        bool isAngle,
                                                        const std::string& currentDir) const {
    namespace fs = std::filesystem;
    auto existsFile = [](const fs::path& p) -> bool {
        std::error_code ec;
        return fs::exists(p, ec) && fs::is_regular_file(p, ec);
    };

    // Quote includes: current file dir first
    if (!isAngle) {
        if (!currentDir.empty()) {
            fs::path p = fs::path(currentDir) / header;
            if (existsFile(p)) return fs::weakly_canonical(p).string();
        }
    }

    // Both forms: search -I paths, then -isystem, then <sysroot>/include/.
    for (const auto& base : includePaths) {
        fs::path p = fs::path(base) / header;
        if (existsFile(p)) return fs::weakly_canonical(p).string();
    }
    for (const auto& base : systemIncludePaths) {
        fs::path p = fs::path(base) / header;
        if (existsFile(p)) return fs::weakly_canonical(p).string();
    }
    if (!sysroot.empty()) {
        fs::path p = fs::path(sysroot) / "include" / header;
        if (existsFile(p)) return fs::weakly_canonical(p).string();
    }

    // As a fallback for quotes, try relative to CWD if no currentDir
    if (!isAngle && currentDir.empty()) {
        fs::path p = fs::path(header);
        if (existsFile(p)) return fs::weakly_canonical(p).string();
    }

    return std::nullopt;
}

bool Preprocessor::isInInclusionStack(const std::string& filePath) const {
    return std::find(inclusionStack.begin(), inclusionStack.end(), filePath) 
        != inclusionStack.end();
}

bool Preprocessor::pushInclusion(const std::string& filePath) {
    if (isInInclusionStack(filePath)) return false;  // Cycle detected
    inclusionStack.push_back(filePath);
    return true;
}

void Preprocessor::popInclusion() {
    if (!inclusionStack.empty()) {
        (void)inclusionStack.back();
        inclusionStack.pop_back();
    }
}

// Non-streaming `run()` removed: use streaming interface (`open()` + `next()`) instead.

std::vector<PPToken> Preprocessor::collectLineTokens(Tokenizer& tokenizer) {
    std::vector<PPToken> tokens;
    while (auto t = tokenizer.peek()) {
        if (t->kind == PPTokenKind::Newline) {
            break;
        }
        tokens.push_back(*tokenizer.next());
    }
    return tokens;
}

void Preprocessor::skipToCloseParen(Tokenizer& tokenizer) {
    while (auto p = tokenizer.peek()) {
        if (p->kind == PPTokenKind::Whitespace) {
            tokenizer.next();
        } else break;
    }
    if (auto close = tokenizer.peek()) {
        if (close->kind == PPTokenKind::Punctuator && close->lexeme == ")") {
            tokenizer.next();
        }
    }
}

bool Preprocessor::handleVariadicParameter(Tokenizer& tokenizer, bool& variadic) {
    variadic = true;
    tokenizer.next();
    skipToCloseParen(tokenizer);
    return true;
}

bool Preprocessor::parseMacroParameters(Tokenizer& tokenizer, std::vector<std::string>& params,
                                        bool& variadic) {
    while (auto param = tokenizer.peek()) {
        if (param->kind == PPTokenKind::Punctuator && param->lexeme == ")") {
            tokenizer.next();
            return true;
        }
        if (param->kind == PPTokenKind::Whitespace) {
            tokenizer.next();
            continue;
        }
        if (param->kind == PPTokenKind::Punctuator && param->lexeme == ",") {
            tokenizer.next();
            continue;
        }
        if (param->kind == PPTokenKind::Punctuator && param->lexeme == "...") {
            return handleVariadicParameter(tokenizer, variadic);
        }
        if (param->kind == PPTokenKind::Identifier) {
            params.push_back(param->lexeme);
            tokenizer.next();
        } else {
            diagnostics.push_back(Diagnostic{
                .message = "invalid parameter in function-like macro",
                .severity = Diagnostic::Severity::Error,
            });
            return false;
        }
    }
    return true;
}

void Preprocessor::trimReplacementWhitespace(std::vector<PPToken>& replacement) {
    while (!replacement.empty() && replacement.front().kind == PPTokenKind::Whitespace) {
        replacement.erase(replacement.begin());
    }
    while (!replacement.empty() && replacement.back().kind == PPTokenKind::Whitespace) {
        replacement.pop_back();
    }
}

bool Preprocessor::handleDefineDirective(Tokenizer& tokenizer) {
    // Skip whitespace after 'define' keyword
    while (auto w = tokenizer.peek()) {
        if (w->kind == PPTokenKind::Whitespace) {
            tokenizer.next();
        } else break;
    }

    auto macroName = tokenizer.peek();
    if (!macroName || macroName->kind != PPTokenKind::Identifier) {
        diagnostics.push_back(Diagnostic{
            .message = "expected macro name after #define",
            .severity = Diagnostic::Severity::Error,
        });
        return false;
    }

    std::string name = macroName->lexeme;
    tokenizer.next();

    bool isFunction = false;
    std::vector<std::string> params;
    bool variadic = false;

    auto next = tokenizer.peek();
    if (next && next->kind == PPTokenKind::Punctuator && next->lexeme == "(") {
        isFunction = true;
        tokenizer.next();
        if (!parseMacroParameters(tokenizer, params, variadic)) {
            return false;
        }
    }

    std::vector<PPToken> replacement = collectLineTokens(tokenizer);
    trimReplacementWhitespace(replacement);

    if (isFunction) {
        macroTable.defineFunctionMacro(name, params, replacement, variadic);
    } else {
        macroTable.defineObjectMacro(name, replacement);
    }

    return true;
}

bool Preprocessor::handleUndefDirective(Tokenizer& tokenizer) {
    // Skip whitespace after 'undef' keyword
    while (auto w = tokenizer.peek()) {
        if (w->kind == PPTokenKind::Whitespace) { tokenizer.next(); }
        else break;
    }

    auto macroName = tokenizer.peek();
    if (!macroName || macroName->kind != PPTokenKind::Identifier) {
        diagnostics.push_back(Diagnostic{
            .message = "expected macro name after #undef",
            .severity = Diagnostic::Severity::Error,
        });
        return false;
    }

    std::string name = macroName->lexeme;
    macroTable.undefine(name);
    tokenizer.next(); // consume macro name

    return true;
}

std::string Preprocessor::stringifyTokens(const std::vector<PPToken>& tokens) {
    std::string result = "\"";
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        
        // Skip leading whitespace
        if (i == 0 && tok.kind == PPTokenKind::Whitespace) continue;
        
        // Skip whitespace unless needed for separation
        if (tok.kind == PPTokenKind::Whitespace) {
            // Only add space if previous token was not punctuation and next is not punctuation
            if (i > 0 && i + 1 < tokens.size() &&
                tokens[i-1].kind != PPTokenKind::Punctuator &&
                tokens[i+1].kind != PPTokenKind::Punctuator) {
                result += " ";
            }
            continue;
        }
        
        // Escape quotes and backslashes in the stringified content
        std::string lexeme = tok.lexeme;
        for (char c : lexeme) {
            if (c == '"' || c == '\\') {
                result += '\\';
            }
            result += c;
        }
    }
    
    result += "\"";
    return result;
}

void Preprocessor::handleOpenParen(std::vector<PPToken>& currentArg, const PPToken& tok, int& parenDepth) {
    parenDepth++;
    currentArg.push_back(tok);
}

bool Preprocessor::handleCloseParen(std::vector<PPToken>& currentArg, std::vector<std::vector<PPToken>>& args,
                                    const PPToken& tok, CloseParenContext& ctx) {
    ctx.parenDepth--;
    if (ctx.parenDepth == 0) {
        if (!currentArg.empty() || args.empty()) {
            args.push_back(currentArg);
        }
        ctx.endIdx = ctx.k;
        return true;
    }
    currentArg.push_back(tok);
    return false;
}

void Preprocessor::handleComma(std::vector<PPToken>& currentArg, std::vector<std::vector<PPToken>>& args) {
    args.push_back(currentArg);
    currentArg.clear();
}

void Preprocessor::collectVariadicArgs(std::vector<std::vector<PPToken>>& args, const Macro* m) {
    if (!m->variadic || args.size() < m->params.size()) return;
    
    std::vector<PPToken> variadicArg;
    for (size_t vIdx = m->params.size(); vIdx < args.size(); ++vIdx) {
        if (vIdx > m->params.size()) {
            variadicArg.push_back(PPToken{
                .kind = PPTokenKind::Punctuator,
                .lexeme = ",",
                .span = {}
            });
        }
        for (const auto& tok : args[vIdx]) {
            variadicArg.push_back(tok);
        }
    }
    args.resize(m->params.size());
    args.push_back(variadicArg);
}

std::vector<std::vector<PPToken>> Preprocessor::collectMacroArguments(
    const std::vector<PPToken>& tokens, size_t startIdx, size_t& endIdx, const Macro* m) {
    
    std::vector<std::vector<PPToken>> args;
    std::vector<PPToken> currentArg;
    int parenDepth = 1;
    
    for (size_t k = startIdx; k < tokens.size(); ++k) {
        const auto& tok = tokens[k];
        
        if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == "(") {
            handleOpenParen(currentArg, tok, parenDepth);
        } else if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == ")") {
            Preprocessor::CloseParenContext ctx{parenDepth, endIdx, k};
            if (handleCloseParen(currentArg, args, tok, ctx)) {
                break;
            }
        } else if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == "," && parenDepth == 1) {
            handleComma(currentArg, args);
        } else if (tok.kind != PPTokenKind::Whitespace || !currentArg.empty()) {
            currentArg.push_back(tok);
        }
    }
    
    collectVariadicArgs(args, m);
    return args;
}

int Preprocessor::findParamIndex(const Macro* m, const PPToken& tok, bool& isVarargs) {
    // Allow matching of parameter tokens even if token kind varies (robustness)
    isVarargs = (m->variadic && tok.lexeme == "__VA_ARGS__");
    if (isVarargs) return -1;

    for (size_t pIdx = 0; pIdx < m->params.size(); ++pIdx) {
        if (tok.lexeme == m->params[pIdx]) {
            return pIdx;
        }
    }
    return -1;
}

std::vector<PPToken> Preprocessor::getArgumentToStringify(const Macro* m, int paramIdx, bool isVarargs,
                                                          const std::vector<std::vector<PPToken>>& args) {
    if (isVarargs && args.size() > m->params.size()) {
        return args[m->params.size()];
    }
    if (paramIdx >= 0 && paramIdx < static_cast<int>(args.size())) {
        return args[paramIdx];
    }
    return {};
}

bool Preprocessor::tryProcessStringification(const Macro* m, size_t& rIdx,
                                             const std::vector<std::vector<PPToken>>& args,
                                             std::vector<PPToken>& substituted) {
    const auto& repl = m->replacement[rIdx];
    if (repl.kind != PPTokenKind::Punctuator || repl.lexeme != "#") {
        return false;
    }
    
    size_t nextIdx = rIdx + 1;
    while (nextIdx < m->replacement.size() && 
           m->replacement[nextIdx].kind == PPTokenKind::Whitespace) {
        nextIdx++;
    }
    
    if (nextIdx >= m->replacement.size()) return false;
    
    const auto& nextTok = m->replacement[nextIdx];
    bool isVarargs = false;
    int paramIdx = findParamIndex(m, nextTok, isVarargs);
    
    if (paramIdx < 0 && !isVarargs) return false;
    
    auto argToStringify = getArgumentToStringify(m, paramIdx, isVarargs, args);
    std::string stringified = stringifyTokens(argToStringify);
    
    PPToken strToken{
        .kind = PPTokenKind::StringLiteral,
        .span = repl.span,
        .lexeme = stringified,
        .paintedMacros = {}
    };
    strToken.paint(m->name);
    // Mark stringification-generated tokens so Phase-5 normalization does not
    // decode their escape sequences (stringification supplies its own escaping).
    strToken.paint("__STRINGIFIED__");
    substituted.push_back(strToken);
    rIdx = nextIdx;
    return true;
}

bool Preprocessor::tryProcessVarArgs(const Macro* m, const PPToken& repl,
                                     const std::vector<std::vector<PPToken>>& args,
                                     std::vector<PPToken>& substituted) {
    if (!m->variadic || repl.kind != PPTokenKind::Identifier || 
        repl.lexeme != "__VA_ARGS__") {
        return false;
    }
    
    if (args.size() > m->params.size()) {
        for (auto vaToken : args[m->params.size()]) {
            vaToken.paint(m->name);
            substituted.push_back(vaToken);
        }
    }
    return true;
}

bool Preprocessor::tryProcessRegularParam(const Macro* m, const PPToken& repl,
                                          const std::vector<std::vector<PPToken>>& args,
                                          std::vector<PPToken>& substituted) {
    for (size_t pIdx = 0; pIdx < m->params.size(); ++pIdx) {
        if (repl.kind == PPTokenKind::Identifier && repl.lexeme == m->params[pIdx]) {
            if (pIdx < args.size()) {
                for (auto arg : args[pIdx]) {
                    arg.paint(m->name);
                    substituted.push_back(arg);
                }
            }
            return true;
        }
    }
    return false;
}

std::vector<PPToken> Preprocessor::substituteParameters(
    const Macro* m, const std::vector<std::vector<PPToken>>& args, const PPToken& invocationToken) {
    
    std::vector<PPToken> substituted;
    
    for (size_t rIdx = 0; rIdx < m->replacement.size(); ++rIdx) {
        const auto& repl = m->replacement[rIdx];
        
        if (tryProcessStringification(m, rIdx, args, substituted)) continue;
        if (tryProcessVarArgs(m, repl, args, substituted)) continue;
        if (tryProcessRegularParam(m, repl, args, substituted)) continue;
        
        // Not a parameter - copy token and paint it
        auto replToken = repl;
        replToken.paint(m->name);
        if (replToken.kind == PPTokenKind::Identifier && replToken.lexeme == "__LINE__") {
            replToken.span = invocationToken.span;
        }
        substituted.push_back(replToken);
    }
    
    return substituted;
}

std::vector<PPToken> Preprocessor::handleTokenPasting(const std::vector<PPToken>& tokens) {
    std::vector<PPToken> result;
    
    for (size_t idx = 0; idx < tokens.size(); ++idx) {
        const auto& tok = tokens[idx];
        
        // Check if current token is ##
        if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == "##") {
            // Find previous non-whitespace token
            int prevIdx = idx - 1;
            while (prevIdx >= 0 && tokens[prevIdx].kind == PPTokenKind::Whitespace) {
                prevIdx--;
            }
            
            // Find next non-whitespace token
            int nextIdx = idx + 1;
            while (nextIdx < static_cast<int>(tokens.size()) && 
                   tokens[nextIdx].kind == PPTokenKind::Whitespace) {
                nextIdx++;
            }
            
            // Paste tokens if we have both previous and next
            if (prevIdx >= 0 && nextIdx < static_cast<int>(tokens.size())) {
                // Remove whitespace before paste operator
                while (!result.empty() && result.back().kind == PPTokenKind::Whitespace) {
                    result.pop_back();
                }
                
                // Paste: concatenate previous and next token lexemes
                if (!result.empty()) {
                    result.back().lexeme += tokens[nextIdx].lexeme;
                    idx = nextIdx;  // Skip to next token
                    continue;
                }
            }
        }
        
        result.push_back(tok);
    }
    
    return result;
}

bool Preprocessor::tryExpandFunctionLikeMacro(
    const std::vector<PPToken>& tokens, size_t& i, const Macro* m, 
    const PPToken& invocationToken, std::vector<PPToken>& result) {
    
    // Find opening '('
    size_t j = i + 1;
    while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) {
        j++;
    }
    
    // Check if we have '(' - if not, don't expand
    if (j >= tokens.size() || tokens[j].kind != PPTokenKind::Punctuator ||
        tokens[j].lexeme != "(") {
        return false;
    }
    
    // Collect arguments
    size_t endIdx = 0;
    auto args = collectMacroArguments(tokens, j + 1, endIdx, m);
    
    // Validate argument count
    if (!m->variadic && args.size() != m->params.size()) {
        return false;
    }
    if (m->variadic && args.size() < m->params.size()) {
        return false;
    }

    // Substitute parameters
    auto substituted = substituteParameters(m, args, invocationToken);
    
    // Handle token pasting
    auto afterPasting = handleTokenPasting(substituted);
    
    // Recursively expand
    auto expanded = expandMacros(afterPasting);
    for (const auto& exp : expanded) {
        result.push_back(exp);
    }
    
    i = endIdx;  // Update position past the closing ')'
    return true;
}

std::vector<PPToken> Preprocessor::expandMacros(const std::vector<PPToken>& tokens) {
    // Paint semantics implementation (C17 §6.10.3.3):
    // Each token tracks a "paint set" of macro names that have already tried to expand it.
    // This prevents recursion while allowing legitimate multi-level expansions.
    
    std::vector<PPToken> result;

    for (size_t i = 0; i < tokens.size(); ++i) {
        auto t = tokens[i];  // Copy to modify paint set

        // Skip expansion for non-identifiers
        if (t.kind != PPTokenKind::Identifier) {
            result.push_back(t);
            continue;
        }

        // Special handling for __LINE__ macro (dynamic expansion)
        if (t.lexeme == "__LINE__") {
            std::string lineStr = std::to_string(t.span.begin.line);
            result.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = t.span,
                .lexeme = lineStr,
                .paintedMacros = {}
            });
            continue;
        }

        // Check if this identifier is a defined macro
        auto macro = macroTable.getMacro(t.lexeme);
        if (!macro) {
            result.push_back(t);
            continue;
        }

        const Macro* m = *macro;

        // Skip if this macro is painted (already tried to expand it)
        if (t.isPainted(m->name)) {
            result.push_back(t);
            continue;
        }

        // Try function-like macro expansion
        if (m->isFunction) {
            if (tryExpandFunctionLikeMacro(tokens, i, m, t, result)) {
                continue;
            }
            // If not expanded (no '(' found), treat as regular identifier
            result.push_back(t);
            continue;
        }

        // Object-like macro expansion
        std::vector<PPToken> substituted;
        for (auto repl : m->replacement) {
            repl.paint(m->name);
            if (repl.kind == PPTokenKind::Identifier && repl.lexeme == "__LINE__") {
                repl.span = t.span;  // Update to invocation site
            }
            substituted.push_back(repl);
        }

        // Recursively expand the substituted tokens
        auto expanded = expandMacros(substituted);
        for (const auto& exp : expanded) {
            result.push_back(exp);
        }
    }

    return result;
}

size_t Preprocessor::skipWhitespaceTokens(const std::vector<PPToken>& tokens, size_t start) {
    while (start < tokens.size() && tokens[start].kind == PPTokenKind::Whitespace) {
        start++;
    }
    return start;
}

std::optional<std::string> Preprocessor::parseDefinedMacroName(const std::vector<PPToken>& tokens, size_t& j, bool hasParens) {
    if (j >= tokens.size() || tokens[j].kind != PPTokenKind::Identifier) {
        diagnostics.push_back(Diagnostic{
            .message = "expected macro name in 'defined'",
            .severity = Diagnostic::Severity::Error,
            .span = (j < tokens.size()) ? std::optional<SourceSpan>(tokens[j].span) : std::optional<SourceSpan>()
        });
        return std::nullopt;
    }
    std::string macroName = tokens[j].lexeme;
    ++j;
    return macroName;
}

bool Preprocessor::validateDefinedCloseParen(const std::vector<PPToken>& tokens, size_t& j) {
    j = skipWhitespaceTokens(tokens, j);
    if (j >= tokens.size() || tokens[j].kind != PPTokenKind::Punctuator || tokens[j].lexeme != ")") {
        diagnostics.push_back(Diagnostic{
            .message = "expected ')' after macro name in 'defined'",
            .severity = Diagnostic::Severity::Error,
            .span = (j < tokens.size()) ? std::optional<SourceSpan>(tokens[j].span) : std::optional<SourceSpan>()
        });
        return false;
    }
    ++j;
    return true;
}

bool Preprocessor::tryParseDefinedOperator(const std::vector<PPToken>& tokens, size_t& i, std::vector<PPToken>& preprocessed) {
    const auto& tok = tokens[i];
    if (tok.kind != PPTokenKind::Identifier || tok.lexeme != "defined") {
        return false;
    }

    size_t j = skipWhitespaceTokens(tokens, i + 1);
    bool hasParens = false;
    if (j < tokens.size() && tokens[j].kind == PPTokenKind::Punctuator && tokens[j].lexeme == "(") {
        hasParens = true;
        ++j;
        j = skipWhitespaceTokens(tokens, j);
    }

    auto macroName = parseDefinedMacroName(tokens, j, hasParens);
    if (!macroName.has_value()) {
        return false;
    }

    if (hasParens && !validateDefinedCloseParen(tokens, j)) {
        return false;
    }

    PPToken folded = tok;
    folded.kind = PPTokenKind::PPNumber;
    folded.lexeme = macroTable.isDefined(*macroName) ? "1" : "0";
    preprocessed.push_back(folded);
    i = j;
    return true;
}

std::optional<int64_t> Preprocessor::evaluateConstantExpression(const std::vector<PPToken>& tokens) {
    std::vector<PPToken> preprocessed;
    preprocessed.reserve(tokens.size());
    for (size_t i = 0; i < tokens.size();) {
        if (tryParseDefinedOperator(tokens, i, preprocessed)) {
            continue;
        }
        preprocessed.push_back(tokens[i]);
        ++i;
    }

    auto expanded = expandMacros(preprocessed);
    ConstExprParser parser(macroTable, diagnostics);
    return parser.evaluate(expanded);
}

bool Preprocessor::handleIfDirective(Tokenizer& tokenizer, const std::vector<PPToken>& dirTokens) {
    auto result = evaluateConstantExpression(dirTokens);
    if (!result) return false;

    ConditionalFrame frame;
    frame.seenTrueBranch = *result != 0;
    // AND in the parent active state (over the frames pushed so far, before
    // this one) so a true #if nested in an inactive block stays inactive —
    // mirrors handleIfdef. checkActiveState() is correct here because the new
    // frame has not been pushed yet.
    frame.currentlyActive = frame.seenTrueBranch && checkActiveState();
    frame.inElse = false;
    conditionalStack.push_back(frame);
    return true;
}

bool Preprocessor::handleIfdefDirective(const std::string& macroName) {
    bool isDefined = macroTable.isDefined(macroName);
    ConditionalFrame frame;
    frame.seenTrueBranch = isDefined;
    frame.currentlyActive = isDefined;
    frame.inElse = false;
    conditionalStack.push_back(frame);
    return true;
}

bool Preprocessor::handleIfndefDirective(const std::string& macroName) {
    bool isDefined = macroTable.isDefined(macroName);
    ConditionalFrame frame;
    frame.seenTrueBranch = !isDefined;
    frame.currentlyActive = !isDefined;
    frame.inElse = false;
    conditionalStack.push_back(frame);
    return true;
}

bool Preprocessor::handleElifDirective(Tokenizer& tokenizer, const std::vector<PPToken>& dirTokens) {
    if (conditionalStack.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#elif without matching #if",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    ConditionalFrame& frame = conditionalStack.back();
    if (frame.inElse) {
        diagnostics.push_back(Diagnostic{
            .message = "#elif after #else",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    if (frame.seenTrueBranch) {
        frame.currentlyActive = false;
        return true;
    }

    auto result = evaluateConstantExpression(dirTokens);
    if (!result) return false;

    bool newActive = *result != 0;
    frame.seenTrueBranch = frame.seenTrueBranch || newActive;
    // Respect the parent block: a true #elif nested inside an inactive block
    // must not re-activate output (checkParentActiveState excludes this frame,
    // which is already on the stack).
    frame.currentlyActive = newActive && checkParentActiveState();
    return true;
}

bool Preprocessor::handleElseDirective() {
    if (conditionalStack.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#else without matching #if",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    ConditionalFrame& frame = conditionalStack.back();
    if (frame.inElse) {
        diagnostics.push_back(Diagnostic{
            .message = "multiple #else in same conditional block",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    frame.inElse = true;
    // Respect the parent block: a #else nested inside an inactive block must
    // not re-activate output even when no prior branch was taken.
    frame.currentlyActive = !frame.seenTrueBranch && checkParentActiveState();
    return true;
}

bool Preprocessor::handleEndifDirective() {
    if (conditionalStack.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#endif without matching #if",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    conditionalStack.pop_back();
    return true;
}

bool Preprocessor::handleErrorDirective(const std::vector<PPToken>& tokens) {
    // #error directive: emit error message and fail preprocessing
    // Collect all tokens (excluding whitespace at boundaries) as the error message
    std::string errorMsg;
    
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Whitespace) {
            if (!errorMsg.empty()) {
                errorMsg += " ";
            }
        } else {
            errorMsg += tok.lexeme;
        }
    }
    
    // Trim trailing whitespace
    while (!errorMsg.empty() && (errorMsg.back() == ' ' || errorMsg.back() == '\t')) {
        errorMsg.pop_back();
    }
    
    diagnostics.push_back(Diagnostic{
        .message = std::string("#error: ") + (errorMsg.empty() ? "(no message)" : errorMsg),
        .severity = Diagnostic::Severity::Error,
        .span = tokens.empty() ? std::nullopt : std::optional<SourceSpan>(tokens[0].span)
    });
    
    return false; // #error always causes preprocessing to fail
}

bool Preprocessor::handleWarningDirective(const std::vector<PPToken>& tokens) {
    // #warning directive: emit warning message but continue preprocessing
    std::string warningMsg;
    
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Whitespace) {
            if (!warningMsg.empty()) {
                warningMsg += " ";
            }
        } else {
            warningMsg += tok.lexeme;
        }
    }
    
    // Trim trailing whitespace
    while (!warningMsg.empty() && (warningMsg.back() == ' ' || warningMsg.back() == '\t')) {
        warningMsg.pop_back();
    }
    
    diagnostics.push_back(Diagnostic{
        .message = std::string("#warning: ") + (warningMsg.empty() ? "(no message)" : warningMsg),
        .severity = Diagnostic::Severity::Warning,
        .span = tokens.empty() ? std::nullopt : std::optional<SourceSpan>(tokens[0].span)
    });
    
    return true; // #warning does not cause preprocessing to fail
}

bool Preprocessor::handleLineDirective(const std::vector<PPToken>& tokens) {
    // #line directive: change line number and optionally filename for subsequent diagnostics
    // Format: #line digit-sequence ["filename"]
    // For now, we'll just validate and accept it without changing our line tracking
    // (Full implementation would require modifying SourcePos in emitted tokens)
    
    if (tokens.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#line directive requires line number",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }
    
    // First token should be a number
    size_t idx = 0;
    while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) {
        idx++;
    }
    
    if (idx >= tokens.size() || tokens[idx].kind != PPTokenKind::PPNumber) {
        diagnostics.push_back(Diagnostic{
            .message = "#line directive requires line number as first argument",
            .severity = Diagnostic::Severity::Error,
            .span = idx < tokens.size() ? std::optional<SourceSpan>(tokens[idx].span) : std::nullopt
        });
        return false;
    }
    
    // Optionally followed by a string literal (filename)
    idx++;
    while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) {
        idx++;
    }
    
    if (idx < tokens.size()) {
        if (tokens[idx].kind != PPTokenKind::StringLiteral) {
            diagnostics.push_back(Diagnostic{
                .message = "#line directive optional second argument must be a string literal",
                .severity = Diagnostic::Severity::Error,
                .span = tokens[idx].span
            });
            return false;
        }
    }
    
    // Note: Full implementation would update line/file tracking here
    // For now, we just validate the syntax
    return true;
}

bool Preprocessor::isPragmaOnceDirective(const std::vector<PPToken>& tokens) {
    if (tokens.empty()) return false;
    
    size_t idx = 0;
    while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) idx++;
    
    if (idx >= tokens.size() || tokens[idx].kind != PPTokenKind::Identifier || 
        tokens[idx].lexeme != "once") {
        return false;
    }
    
    idx++;
    while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) idx++;
    
    if (idx < tokens.size()) return false;
    
    if (!inclusionStack.empty()) {
        pragmaOnceFiles.insert(inclusionStack.back());
        (void)inclusionStack.back();
    }
    return true;
}

std::string Preprocessor::buildPragmaContent(const std::vector<PPToken>& tokens) {
    std::string pragmaContent;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Whitespace) {
            if (!pragmaContent.empty()) {
                pragmaContent += " ";
            }
        } else {
            pragmaContent += tok.lexeme;
        }
    }
    return pragmaContent;
}

void Preprocessor::trimTrailingWhitespace(std::string& content) {
    while (!content.empty() && (content.back() == ' ' || content.back() == '\t')) {
        content.pop_back();
    }
}

// Normalize a literal token: decode escape sequences and UCNs inside string literal tokens.
// Keeps prefix and surrounding quotes, but replaces inner content with decoded bytes
// encoded as UTF-8 for execution character set. Emits diagnostics on malformed escapes.
// Note: character constants are left unchanged here so the parser sees the original
// source lexeme (tests and downstream code expect the raw char-constant form).
void Preprocessor::normalizeLiteralToken(PPToken& tok) {
    // Normalize both string literals and character constants. Skip tokens
    // generated by the stringification operator (#), which already contain
    // their escaped representation and must not be decoded.
    if (!(tok.kind == PPTokenKind::StringLiteral || tok.kind == PPTokenKind::CharConst)) return;
    if (tok.kind == PPTokenKind::StringLiteral && tok.isPainted("__STRINGIFIED__")) return;
    const std::string &orig = tok.lexeme;
    if (orig.empty()) return;

    // Extract prefix (u8, u, U, L) if present
    size_t i = 0;
    std::string prefix;
    if (orig.size() >= 2) {
        // u8 prefix is two chars
        if (orig.size() >= 3 && orig[0] == 'u' && orig[1] == '8') { prefix = "u8"; i = 2; }
        else if (orig[0] == 'u' || orig[0] == 'U' || orig[0] == 'L') { prefix = std::string(1, orig[0]); i = 1; }
    }

    if (i >= orig.size()) return;
    char quote = orig[i];
    if (!(quote == '"' || quote == '\'')) return;
    // find closing quote (last character expected to be same quote)
    if (orig.size() < i + 2) return;
    size_t end = orig.size() - 1;
    if (orig[end] != quote) return;

    std::string inner = orig.substr(i + 1, end - (i + 1));
    std::string out;
    out.reserve(inner.size());

    auto push_codepoint_utf8 = [&](uint32_t cp) {
        if (cp <= 0x7F) out.push_back(static_cast<char>(cp));
        else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            if (cp > 0x10FFFF) cp = 0xFFFD;
            out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    };

    for (size_t p = 0; p < inner.size();) {
        char c = inner[p];
        if (c != '\\') {
            out.push_back(c);
            ++p;
            continue;
        }
        // backslash escape
        ++p;
        if (p >= inner.size()) {
            diagnostics.push_back(Diagnostic{.message = "trailing backslash in literal", .severity = Diagnostic::Severity::Error, .span = tok.span});
            break;
        }
        char e = inner[p++];
        if (e == 'n') { out.push_back('\n'); out.back() = '\n'; continue; }
        if (e == 't') { out.push_back('\t'); out.back() = '\t'; continue; }
        if (e == 'r') { out.push_back('\r'); out.back() = '\r'; continue; }
        if (e == 'a') { out.push_back(static_cast<char>(0x07)); continue; }
        if (e == 'b') { out.push_back(static_cast<char>(0x08)); continue; }
        if (e == 'f') { out.push_back(static_cast<char>(0x0C)); continue; }
        if (e == 'v') { out.push_back(static_cast<char>(0x0B)); continue; }
        if (e == '\\') { out.push_back('\\'); out.back() = '\\'; continue; }
        if (e == '"') { out.push_back('"'); continue; }
        if (e == '\'') { out.push_back('\''); continue; }
        if (e == '?') { out.push_back('?'); continue; }

        if (e == 'x') {
            // hex sequence: one or more hex digits
            size_t start = p;
            uint32_t val = 0;
            while (p < inner.size() && isxdigit(static_cast<unsigned char>(inner[p]))) {
                char ch = inner[p++];
                val = (val << 4) + (uint32_t)( (ch>='0'&&ch<='9') ? ch - '0' : (ch>='a'&&ch<='f') ? 10 + ch - 'a' : 10 + ch - 'A');
            }
            if (p == start) {
                diagnostics.push_back(Diagnostic{.message = "\'\\x\' escape with no hex digits", .severity = Diagnostic::Severity::Error, .span = tok.span});
            }
            push_codepoint_utf8(val);
            continue;
        }

        if (e == 'u' || e == 'U') {
            int need = (e == 'u') ? 4 : 8;
            uint32_t val = 0;
            int consumed = 0;
            while (consumed < need && p < inner.size() && isxdigit(static_cast<unsigned char>(inner[p]))) {
                char ch = inner[p++]; ++consumed;
                val = (val << 4) + (uint32_t)( (ch>='0'&&ch<='9') ? ch - '0' : (ch>='a'&&ch<='f') ? 10 + ch - 'a' : 10 + ch - 'A');
            }
            if (consumed != need) {
                diagnostics.push_back(Diagnostic{.message = "truncated Unicode escape", .severity = Diagnostic::Severity::Error, .span = tok.span});
            }
            // replace surrogates and out-of-range
            if (val >= 0xD800 && val <= 0xDFFF) val = 0xFFFD;
            if (val > 0x10FFFF) val = 0xFFFD;
            push_codepoint_utf8(val);
            continue;
        }

        // octal escape: up to 3 octal digits, including the digit we already saw if it was octal
        if (e >= '0' && e <= '7') {
            uint32_t val = (uint32_t)(e - '0');
            int cnt = 1;
            while (cnt < 3 && p < inner.size() && inner[p] >= '0' && inner[p] <= '7') {
                val = (val << 3) + (uint32_t)(inner[p++] - '0');
                ++cnt;
            }
            push_codepoint_utf8(val);
            continue;
        }

        // default: unknown escape, emit the character itself
        out.push_back(e);
    }
    // For string literals, rebuild lexeme and attach decoded metadata
    if (tok.kind == PPTokenKind::StringLiteral) {
        std::string rebuilt = prefix + std::string(1, quote) + out + std::string(1, quote);
        tok.lexeme = rebuilt;
        tok.decodedString = out;
        return;
    }

    // For character constants, compute numeric value by packing decoded bytes.
    if (tok.kind == PPTokenKind::CharConst) {
        std::string rebuilt = prefix + std::string(1, quote) + out + std::string(1, quote);
        tok.lexeme = rebuilt;
        unsigned int acc = 0;
        for (unsigned char uc : out) acc = (acc << 8) | static_cast<unsigned int>(uc);
        tok.decodedCharValue = acc;
        return;
    }
}

// Helpers to push tokens into the output buffer while performing Phase 6
// adjacent string literal concatenation when applicable.
static std::string extractLiteralPrefix(const std::string &lex) {
    size_t i = 0;
    if (lex.size() >= 3 && lex[0] == 'u' && lex[1] == '8') return "u8";
    if (lex.size() >= 2 && (lex[0] == 'u' || lex[0] == 'U' || lex[0] == 'L')) return std::string(1, lex[0]);
    return std::string();
}

static bool extractLiteralInnerAndQuote(const std::string &lex, std::string &outInner, char &outQuote) {
    outInner.clear(); outQuote = 0;
    if (lex.empty()) return false;
    size_t i = 0;
    if (lex.size() >= 3 && lex[0] == 'u' && lex[1] == '8') i = 2;
    else if (lex.size() >= 2 && (lex[0] == 'u' || lex[0] == 'U' || lex[0] == 'L')) i = 1;
    if (i >= lex.size()) return false;
    char q = lex[i];
    if (!(q == '"' || q == '\'')) return false;
    if (lex.size() < i + 2) return false;
    if (lex.back() != q) return false;
    outQuote = q;
    outInner = lex.substr(i + 1, lex.size() - (i + 2));
    return true;
}

// combine prefixes according to C rules (simplified):
// - identical non-empty prefixes -> that prefix
// - one empty and one non-empty -> the non-empty
// - both empty -> empty
// - different non-empty prefixes -> incompatible

void Preprocessor::pushTokenBackWithConcat(const PPToken& tok) {
    // Fast path: non-string literals just append
    if (tok.kind != PPTokenKind::StringLiteral) {
        outBuffer.push_back(tok);
        return;
    }

    // Try to merge with a previous string literal if present.
    int prevIdx = static_cast<int>(outBuffer.size()) - 1;
    while (prevIdx >= 0 && (outBuffer[prevIdx].kind == PPTokenKind::Whitespace || outBuffer[prevIdx].kind == PPTokenKind::Newline)) {
        --prevIdx;
    }

    if (prevIdx >= 0 && outBuffer[prevIdx].kind == PPTokenKind::StringLiteral) {
        PPToken left = outBuffer[prevIdx];
        PPToken right = tok;
        normalizeLiteralToken(left);
        normalizeLiteralToken(right);

        std::string lpre = extractLiteralPrefix(left.lexeme);
        std::string rpre = extractLiteralPrefix(right.lexeme);
        std::string lin, rin; char lq=0, rq=0;
        if (extractLiteralInnerAndQuote(left.lexeme, lin, lq) && extractLiteralInnerAndQuote(right.lexeme, rin, rq) && lq == rq) {
            std::string combinedPrefix;
            if (lpre.empty() && rpre.empty()) combinedPrefix = std::string();
            else if (lpre.empty()) combinedPrefix = rpre;
            else if (rpre.empty()) combinedPrefix = lpre;
            else if (lpre == rpre) combinedPrefix = lpre;
            else {
                diagnostics.push_back(Diagnostic{.message = "incompatible string literal prefixes during concatenation", .severity = Diagnostic::Severity::Error, .span = std::nullopt});
                outBuffer.push_back(tok);
                return;
            }

            size_t keep = static_cast<size_t>(prevIdx) + 1;
            while (outBuffer.size() > keep) outBuffer.pop_back();

            std::string newInner = lin + rin;
            std::string rebuilt = combinedPrefix + std::string(1, lq) + newInner + std::string(1, lq);
            outBuffer.back().lexeme = rebuilt;
            outBuffer.back().span.end = tok.span.end;
            outBuffer.back().copyPaint(tok);
            return;
        }
        // fallthrough to lookahead
    }

    // No previous merge occurred. If we're in the middle of a batch push
    // (handleInclude / macro expansion), DO NOT read from the parent
    // tokenizer — the next raw token belongs to source that follows the
    // batch and would get spliced into the batch's literal. Just append.
    PPToken merged = tok;
    normalizeLiteralToken(merged);
    if (batchPushNoLookahead) {
        outBuffer.push_back(merged);
        return;
    }

    // Otherwise perform tokenizer lookahead to merge following adjacent
    // literals (Phase 6: `"foo" "bar"` → `"foobar"`).
    std::vector<PPToken> saved; // whitespace/newline tokens between literals

    while (true) {
        auto ntOpt = readRawToken();
        if (!ntOpt) break;
        PPToken nt = *ntOpt;

        if (nt.kind == PPTokenKind::Whitespace || nt.kind == PPTokenKind::Newline) {
            saved.push_back(nt);
            continue;
        }

        if (nt.kind != PPTokenKind::StringLiteral) {
            outBuffer.push_back(merged);
            for (const auto &s : saved) outBuffer.push_back(s);
            outBuffer.push_back(nt);
            return;
        }

        PPToken right = nt;
        normalizeLiteralToken(right);

        std::string lpre = extractLiteralPrefix(merged.lexeme);
        std::string rpre = extractLiteralPrefix(right.lexeme);
        std::string lin, rin; char lq=0, rq=0;
        if (!extractLiteralInnerAndQuote(merged.lexeme, lin, lq) || !extractLiteralInnerAndQuote(right.lexeme, rin, rq) || lq != rq) {
            outBuffer.push_back(merged);
            for (const auto &s : saved) outBuffer.push_back(s);
            outBuffer.push_back(right);
            return;
        }

        std::string combinedPrefix;
        if (lpre.empty() && rpre.empty()) combinedPrefix = std::string();
        else if (lpre.empty()) combinedPrefix = rpre;
        else if (rpre.empty()) combinedPrefix = lpre;
        else if (lpre == rpre) combinedPrefix = lpre;
        else {
            diagnostics.push_back(Diagnostic{.message = "incompatible string literal prefixes during concatenation", .severity = Diagnostic::Severity::Error, .span = std::nullopt});
            outBuffer.push_back(merged);
            for (const auto &s : saved) outBuffer.push_back(s);
            outBuffer.push_back(right);
            return;
        }

        std::string newInner = lin + rin;
        std::string rebuilt = combinedPrefix + std::string(1, lq) + newInner + std::string(1, lq);
        merged.lexeme = rebuilt;
        merged.span.end = right.span.end;
        merged.copyPaint(right);
        saved.clear();
        // continue to merge more literals
    }

    // EOF reached: push merged token and any saved whitespace
    outBuffer.push_back(merged);
    for (const auto &s : saved) outBuffer.push_back(s);
}

void Preprocessor::pushTokenFrontWithConcat(const PPToken& tok) {
    // Only attempt special Phase-6 behavior for string literals; otherwise push normally.
    if (tok.kind != PPTokenKind::StringLiteral) { outBuffer.push_front(tok); return; }

    // If there's an existing front literal (skipping whitespace), try to merge with it first.
    size_t j = 0;
    while (j < outBuffer.size() && (outBuffer[j].kind == PPTokenKind::Whitespace || outBuffer[j].kind == PPTokenKind::Newline)) ++j;
    if (j < outBuffer.size() && outBuffer[j].kind == PPTokenKind::StringLiteral) {
        PPToken left = tok;
        PPToken right = outBuffer[j];
        normalizeLiteralToken(left);
        normalizeLiteralToken(right);

        std::string lpre = extractLiteralPrefix(left.lexeme);
        std::string rpre = extractLiteralPrefix(right.lexeme);
        std::string lin, rin; char lq=0, rq=0;
        if (extractLiteralInnerAndQuote(left.lexeme, lin, lq) && extractLiteralInnerAndQuote(right.lexeme, rin, rq) && lq == rq) {
            std::string combinedPrefix;
            if (lpre.empty() && rpre.empty()) combinedPrefix = std::string();
            else if (lpre.empty()) combinedPrefix = rpre;
            else if (rpre.empty()) combinedPrefix = lpre;
            else if (lpre == rpre) combinedPrefix = lpre;
            else {
                diagnostics.push_back(Diagnostic{.message = "incompatible string literal prefixes during concatenation", .severity = Diagnostic::Severity::Error, .span = std::nullopt});
                outBuffer.push_front(tok);
                return;
            }

            // Remove leading whitespace/newline tokens before the target
            for (size_t k = 0; k < j; ++k) outBuffer.pop_front();

            // Build combined lexeme and update front element
            std::string newInner = lin + rin;
            std::string rebuilt = combinedPrefix + std::string(1, lq) + newInner + std::string(1, lq);
            outBuffer.front().lexeme = rebuilt;
            outBuffer.front().span.begin = tok.span.begin;
            outBuffer.front().copyPaint(tok);
            return;
        }
    }

    // Otherwise, perform lookahead on the raw token stream to merge following literals
    PPToken merged = tok;
    normalizeLiteralToken(merged);
    std::vector<PPToken> saved; // whitespace/newline tokens between literals

    while (true) {
        auto ntOpt = readRawToken();
        if (!ntOpt) break;
        PPToken nt = *ntOpt;

        if (nt.kind == PPTokenKind::Whitespace || nt.kind == PPTokenKind::Newline) {
            saved.push_back(nt);
            continue;
        }

        if (nt.kind != PPTokenKind::StringLiteral) {
            // Insert nt, saved, and merged at front so order becomes: merged, saved..., nt, previous...
            outBuffer.push_front(nt);
            for (auto it = saved.rbegin(); it != saved.rend(); ++it) outBuffer.push_front(*it);
            outBuffer.push_front(merged);
            return;
        }

        PPToken right = nt;
        normalizeLiteralToken(right);

        std::string lpre = extractLiteralPrefix(merged.lexeme);
        std::string rpre = extractLiteralPrefix(right.lexeme);
        std::string lin, rin; char lq=0, rq=0;
        if (!extractLiteralInnerAndQuote(merged.lexeme, lin, lq) || !extractLiteralInnerAndQuote(right.lexeme, rin, rq) || lq != rq) {
            // cannot merge: push right then saved then merged
            outBuffer.push_front(right);
            for (auto it = saved.rbegin(); it != saved.rend(); ++it) outBuffer.push_front(*it);
            outBuffer.push_front(merged);
            return;
        }

        std::string combinedPrefix;
        if (lpre.empty() && rpre.empty()) combinedPrefix = std::string();
        else if (lpre.empty()) combinedPrefix = rpre;
        else if (rpre.empty()) combinedPrefix = lpre;
        else if (lpre == rpre) combinedPrefix = lpre;
        else {
            diagnostics.push_back(Diagnostic{.message = "incompatible string literal prefixes during concatenation", .severity = Diagnostic::Severity::Error, .span = std::nullopt});
            outBuffer.push_front(right);
            for (auto it = saved.rbegin(); it != saved.rend(); ++it) outBuffer.push_front(*it);
            outBuffer.push_front(merged);
            return;
        }

        std::string newInner = lin + rin;
        std::string rebuilt = combinedPrefix + std::string(1, lq) + newInner + std::string(1, lq);
        merged.lexeme = rebuilt;
        merged.span.end = right.span.end;
        merged.copyPaint(right);
        saved.clear();
        // continue to attempt merging more literals
    }

    // EOF: push merged and saved (merged first)
    for (auto it = saved.rbegin(); it != saved.rend(); ++it) outBuffer.push_front(*it);
    outBuffer.push_front(merged);
}

    

bool Preprocessor::handlePragmaDirective(const std::vector<PPToken>& tokens) {
    bool isPragmaOnce = isPragmaOnceDirective(tokens);
    
    if (!isPragmaOnce) {
        std::string pragmaContent = buildPragmaContent(tokens);
        trimTrailingWhitespace(pragmaContent);
        
        diagnostics.push_back(Diagnostic{
            .message = std::string("#pragma: ") + (pragmaContent.empty() ? "(empty)" : pragmaContent),
            .severity = Diagnostic::Severity::Warning,
            .span = tokens.empty() ? std::nullopt : std::optional<SourceSpan>(tokens[0].span)
        });
    }
    
    return true;
}

} // namespace wvmcc
