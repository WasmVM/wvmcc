#include <cstdint>
#include "Preprocessor.hpp"
#include "ConstExprParser.hpp"

#include <algorithm>
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
    // Normalize the buffered token in place so callers always see decoded
    // literals. Normalization is idempotent (guarded by the decoded metadata),
    // so a later peek() or next() on the same token does not decode it again —
    // decoding twice would corrupt escaped lexemes and re-emit any range
    // diagnostics a malformed literal produced.
    normalizeLiteralToken(outBuffer.front());
    return outBuffer.front();
}

std::optional<PPToken> Preprocessor::next() {
    ensureBuffer();
    if (outBuffer.empty()) return std::nullopt;
    auto t = outBuffer.front(); outBuffer.pop_front();

    // If this is a string literal, perform Phase 6 adjacent literal concatenation.
    if (t.kind == PPTokenKind::StringLiteral) {
        normalizeLiteralToken(t);
        // Pull following tokens on demand and concatenate adjacent string
        // literals. Crucially this expands macros that follow a string literal
        // (`"%" PRIx8` → `"%" "8"` → `"%8"`) before deciding whether they
        // concatenate, because ensureBuffer performs macro expansion. White
        // space and new-lines between adjacent literals are not significant
        // (C 6.4.5) and are discarded.
        while (true) {
            // Advance to the next significant (non-whitespace) buffered token,
            // pulling and macro-expanding more input as needed.
            bool haveFront = false;
            while (true) {
                ensureBuffer();
                if (outBuffer.empty()) break;          // EOF
                auto k = outBuffer.front().kind;
                if (k == PPTokenKind::Whitespace || k == PPTokenKind::Newline) {
                    outBuffer.pop_front();
                    continue;
                }
                haveFront = true;
                break;
            }
            if (!haveFront) break;                       // nothing more to concat
            if (outBuffer.front().kind != PPTokenKind::StringLiteral) break;

            PPToken right = outBuffer.front();
            normalizeLiteralToken(right);

            std::string lpre = extractLiteralPrefix(t.lexeme);
            std::string rpre = extractLiteralPrefix(right.lexeme);
            std::string lin, rin; char lq=0, rq=0;
            if (!extractLiteralInnerAndQuote(t.lexeme, lin, lq) || !extractLiteralInnerAndQuote(right.lexeme, rin, rq)) {
                break; // malformed: stop merging
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
                // Leave `right` buffered; it is returned by the next call.
                break;
            }

            // Commit: consume `right` and merge its inner into `t`. `lin`/`rin`
            // are already-decoded inners (both operands were normalized), so the
            // rebuilt lexeme and decodedString are kept consistent — otherwise a
            // stale decodedString from the left operand would be returned
            // verbatim, dropping the concatenation.
            outBuffer.pop_front();
            std::string newInner = lin + rin;
            t.lexeme = combinedPrefix + std::string(1, lq) + newInner + std::string(1, lq);
            t.decodedString = newInner;
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
    std::string contents = ss.str();
    auto ssp = std::make_unique<std::istringstream>(contents);
    auto tz = std::make_unique<Tokenizer>(*ssp);
    FileCtx ctx;
    ctx.path = path;
    ctx.stream = std::move(ssp);
    ctx.tokenizer = std::move(tz);
    // #28: register this file so its tokens can be stamped with a stable fileId
    // and its text recovered for caret rendering at diagnostic-print time.
    ctx.fileId = sourceManager_->addFile(path, std::move(contents));
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
        // #28: the Tokenizer's per-file SourceBuffer leaves fileId at 0; stamp the
        // owning file's id here, the single choke point where tokens enter from a
        // file, so spans carry it through Lexer -> Parser -> AST unchanged.
        tok->span.begin.fileId = ctx.fileId;
        tok->span.end.fileId = ctx.fileId;
        return tok;
    }
    return std::nullopt;
}

void Preprocessor::ensureBuffer() {
    // Keep reading tokens until we have at least one token to return.
    // Additionally, if the last token we produced is a string literal,
    // continue reading to allow adjacent string-literal concatenation
    // to occur before we return any token.
    // Produce exactly one logical token (one ensureBuffer call may read several
    // raw tokens — directives, whitespace, empty macro expansions — before one
    // surfaces). We deliberately do NOT buffer ahead past a string literal here:
    // adjacent-literal concatenation (Phase 6) is driven by next(), which pulls
    // and macro-expands following tokens on demand. Buffering ahead while the
    // tail is a string literal collided with macro expansion's front-push and
    // left a macro after a string literal unexpanded (issue #90 gap 1).
    while (true) {
        if (!outBuffer.empty()) {
            // The _Pragma operator (6.10.9) is executed here, the single point
            // every token passes through before surfacing — whether read from
            // a file or produced by macro expansion (expansion results are
            // pushed into outBuffer, bypassing the raw-token path below).
            const PPToken& front = outBuffer.front();
            if (front.kind == PPTokenKind::Identifier && front.lexeme == "_Pragma") {
                PPToken opTok = front;
                outBuffer.pop_front();
                handlePragmaOperator(opTok);
                continue;   // outBuffer may be empty again; keep producing
            }
            return;
        }

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
        handleDirective(/*inactiveConditional=*/true);
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
    if (tryExpandFunctionLikeMacro(invocation, idx, m, tok, expandedResult)) {
        for (const auto &et : expandedResult) pushTokenBackWithConcat(et);
    } else {
        for (const auto &itok : invocation) pushTokenBackWithConcat(itok);
    }
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
    std::string lineStr = std::to_string((long)tok.span.begin.line + lineOffset_);
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

void Preprocessor::handleDirective(bool inactiveConditional) {
    auto nameOpt = readDirectiveName();
    if (!nameOpt.has_value()) return;
    std::string dir = *nameOpt;

    // In a skipped conditional branch only the directives that manage the
    // conditional nesting are honored (C 6.10p6); #define/#undef/#include/#error
    // and the like must be ignored so they never take effect on the not-taken
    // path. (#ifdef/#ifndef route through handleSimpleDirective below.)
    if (inactiveConditional && dir != "if" && dir != "ifdef" && dir != "ifndef"
        && dir != "elif" && dir != "else" && dir != "endif") {
        skipRestOfDirectiveTokens();
        return;
    }

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
    // pushTokenBackWithConcat no longer reads from the tokenizer, so the
    // included tokens are appended as-is; adjacent-literal concatenation across
    // the include boundary is handled later by next()'s on-demand pull.
    for (const auto &tk : temp) pushTokenBackWithConcat(tk);
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
        // #28: share the file registry so the included file's tokens get globally
        // unique fileIds and its text stays recoverable after the child is gone.
        child.sourceManager_ = sourceManager_;
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

// 6.10.8p2: none of the predefined macro names, nor the identifier `defined`,
// shall be the subject of a #define or #undef directive. Mirrors the set
// installed by pushFile() (plus the dynamically maintained __LINE__/__FILE__).
static bool isProtectedMacroName(const std::string& name) {
    static const std::unordered_set<std::string> names = {
        "defined", "__LINE__", "__FILE__", "__DATE__", "__TIME__",
        "__STDC__", "__STDC_VERSION__", "__STDC_HOSTED__",
        "__STDC_NO_ATOMICS__", "__STDC_NO_COMPLEX__", "__STDC_NO_THREADS__",
    };
    return names.count(name) != 0;
}

// 6.10.3p1: two replacement lists are identical when the token spellings match
// and each pair of adjacent tokens is separated by white space in both lists
// or in neither (all white-space separations are considered identical).
static bool replacementListsIdentical(const std::vector<PPToken>& a,
                                      const std::vector<PPToken>& b) {
    auto normalize = [](const std::vector<PPToken>& list) {
        std::vector<std::pair<std::string, bool>> out; // (spelling, preceded-by-ws)
        bool ws = false;
        for (const auto& t : list) {
            if (t.kind == PPTokenKind::Whitespace) { ws = true; continue; }
            out.emplace_back(t.lexeme, ws && !out.empty());
            ws = false;
        }
        return out;
    };
    return normalize(a) == normalize(b);
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
    if (isProtectedMacroName(name)) {
        diagnostics.push_back(Diagnostic{
            .message = "'" + name + "' cannot be the subject of #define (6.10.8p2)",
            .severity = Diagnostic::Severity::Error,
            .span = macroName->span,
        });
        return false;
    }
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

    // 6.10.3.2p1: in a function-like macro, each '#' in the replacement list
    // must be followed by a parameter (or __VA_ARGS__ when variadic). This is
    // a constraint on the *definition*, so diagnose it here — not at expansion
    // time, which would miss macros that are never expanded.
    if (isFunction) {
        for (size_t k = 0; k < replacement.size(); ++k) {
            const auto& t = replacement[k];
            if (t.kind != PPTokenKind::Punctuator || t.lexeme != "#") continue;
            size_t n = k + 1;
            while (n < replacement.size() && replacement[n].kind == PPTokenKind::Whitespace) ++n;
            bool followsParam = n < replacement.size()
                && replacement[n].kind == PPTokenKind::Identifier
                && (std::find(params.begin(), params.end(), replacement[n].lexeme) != params.end()
                    || (variadic && replacement[n].lexeme == "__VA_ARGS__"));
            if (!followsParam) {
                diagnostics.push_back(Diagnostic{
                    .message = "'#' is not followed by a macro parameter in the definition of '" + name + "' (6.10.3.2p1)",
                    .severity = Diagnostic::Severity::Error,
                    .span = t.span,
                });
                return false;
            }
        }
    }

    // 6.10.3p2: a currently defined macro may be redefined only by an
    // identical definition (same kind, parameters, and replacement list).
    if (auto prev = macroTable.getMacro(name)) {
        const Macro* m = *prev;
        bool identical = m->isFunction == isFunction
                      && m->variadic == variadic
                      && m->params == params
                      && replacementListsIdentical(m->replacement, replacement);
        if (!identical) {
            diagnostics.push_back(Diagnostic{
                .message = "macro '" + name + "' redefined with a different replacement list (6.10.3p2)",
                .severity = Diagnostic::Severity::Error,
                .span = macroName->span,
            });
            return false;
        }
    }

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
    if (isProtectedMacroName(name)) {
        diagnostics.push_back(Diagnostic{
            .message = "'" + name + "' cannot be the subject of #undef (6.10.8p2)",
            .severity = Diagnostic::Severity::Error,
            .span = macroName->span,
        });
        return false;
    }
    macroTable.undefine(name);
    tokenizer.next(); // consume macro name

    return true;
}

std::string Preprocessor::stringifyTokens(const std::vector<PPToken>& tokens) {
    // 6.10.3.2p2: white space before the first and after the last token is
    // deleted; every other white-space sequence between tokens becomes a single
    // space. (The previous logic suppressed the space whenever a neighbor was
    // punctuation, so `flag == 1` stringized to `flag==1`.)
    std::string result = "\"";
    bool pendingSpace = false;
    bool emittedAny = false;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Whitespace) {
            if (emittedAny) pendingSpace = true;   // internal run → one space (deferred)
            continue;                               // leading run dropped (emittedAny false)
        }
        if (pendingSpace) { result += ' '; pendingSpace = false; }
        // Escape " and \ so the result is a well-formed string literal.
        for (char c : tok.lexeme) {
            if (c == '"' || c == '\\') result += '\\';
            result += c;
        }
        emittedAny = true;                          // trailing ws never flushes pendingSpace
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
                .span = {},
                .lexeme = ","
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
    // Because normalization is skipped, set the decoded runtime value here:
    // strip the surrounding quotes and undo the `\"`/`\\` escaping stringifyTokens
    // added. Otherwise the lexer falls back to the raw lexeme (quotes included),
    // so `#expr` reached the program as `"expr"` rather than `expr`.
    {
        std::string decoded;
        if (stringified.size() >= 2) {
            const std::string inner = stringified.substr(1, stringified.size() - 2);
            for (size_t k = 0; k < inner.size(); ++k) {
                if (inner[k] == '\\' && k + 1 < inner.size() &&
                    (inner[k + 1] == '"' || inner[k + 1] == '\\')) {
                    decoded += inner[++k];
                } else {
                    decoded += inner[k];
                }
            }
        }
        strToken.decodedString = decoded;
    }
    substituted.push_back(strToken);
    rIdx = nextIdx;
    return true;
}

// True if the replacement-list token at `rIdx` is an operand of a `##` paste
// (its nearest significant neighbour on either side is `##`). C 6.10.3.1p1: a
// parameter that is such an operand is substituted by its *unexpanded* argument;
// otherwise the argument is macro-expanded first.
static bool isPasteAdjacent(const Macro* m, size_t rIdx) {
    const auto& repl = m->replacement;
    long p = static_cast<long>(rIdx) - 1;
    while (p >= 0 && repl[p].kind == PPTokenKind::Whitespace) --p;
    if (p >= 0 && repl[p].kind == PPTokenKind::Punctuator && repl[p].lexeme == "##") return true;
    size_t n = rIdx + 1;
    while (n < repl.size() && repl[n].kind == PPTokenKind::Whitespace) ++n;
    if (n < repl.size() && repl[n].kind == PPTokenKind::Punctuator && repl[n].lexeme == "##") return true;
    return false;
}

// Append `argTokens` to `substituted`, macro-expanding them first unless this
// parameter is a `##` operand (which uses the raw argument), then painting the
// result with `m` so the outer macro is not re-entered during the later rescan.
void Preprocessor::appendSubstitutedArgument(const Macro* m, bool pasteAdjacent,
                                             const std::vector<PPToken>& argTokens,
                                             std::vector<PPToken>& substituted) {
    if (pasteAdjacent) {
        for (auto arg : argTokens) {
            arg.paint(m->name);
            substituted.push_back(arg);
        }
        return;
    }
    // C 6.10.3.1p1: replace the parameter with its argument "after all macros
    // contained therein have been expanded" (argument prescan).
    auto expanded = expandMacros(argTokens);
    for (auto arg : expanded) {
        arg.paint(m->name);
        substituted.push_back(arg);
    }
}

bool Preprocessor::tryProcessVarArgs(const Macro* m, const PPToken& repl, bool pasteAdjacent,
                                     const std::vector<std::vector<PPToken>>& args,
                                     std::vector<PPToken>& substituted) {
    if (!m->variadic || repl.kind != PPTokenKind::Identifier ||
        repl.lexeme != "__VA_ARGS__") {
        return false;
    }

    if (args.size() > m->params.size()) {
        appendSubstitutedArgument(m, pasteAdjacent, args[m->params.size()], substituted);
    }
    return true;
}

bool Preprocessor::tryProcessRegularParam(const Macro* m, const PPToken& repl, bool pasteAdjacent,
                                          const std::vector<std::vector<PPToken>>& args,
                                          std::vector<PPToken>& substituted) {
    for (size_t pIdx = 0; pIdx < m->params.size(); ++pIdx) {
        if (repl.kind == PPTokenKind::Identifier && repl.lexeme == m->params[pIdx]) {
            if (pIdx < args.size()) {
                appendSubstitutedArgument(m, pasteAdjacent, args[pIdx], substituted);
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
        bool pasteAdjacent = isPasteAdjacent(m, rIdx);
        if (tryProcessVarArgs(m, repl, pasteAdjacent, args, substituted)) continue;
        if (tryProcessRegularParam(m, repl, pasteAdjacent, args, substituted)) continue;
        
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
            std::string lineStr = std::to_string((long)t.span.begin.line + lineOffset_);
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
    
    // The number token is on the same physical line as the directive; the line
    // that *follows* the directive is renumbered to `n` (6.10.4p1).
    size_t numIdx = idx - 1;
    while (numIdx > 0 && tokens[numIdx].kind == PPTokenKind::Whitespace) numIdx--;
    long n = 0;
    try { n = std::stol(tokens[numIdx].lexeme); } catch (...) { n = 0; }
    long directiveLine = (long)tokens[numIdx].span.begin.line;
    lineOffset_ = n - directiveLine - 1;            // physicalLine + offset == presumed

    if (idx < tokens.size()) {
        if (tokens[idx].kind != PPTokenKind::StringLiteral) {
            diagnostics.push_back(Diagnostic{
                .message = "#line directive optional second argument must be a string literal",
                .severity = Diagnostic::Severity::Error,
                .span = tokens[idx].span
            });
            return false;
        }
        // Redefine __FILE__ to the presumed filename. Mirror the initial
        // definition: a StringLiteral token whose lexeme keeps its quotes
        // (normalization later decodes it for use).
        std::vector<PPToken> replacement;
        replacement.push_back(PPToken{ .kind = PPTokenKind::StringLiteral,
                                       .span = tokens[idx].span,
                                       .lexeme = tokens[idx].lexeme,
                                       .paintedMacros = {} });
        macroTable.defineObjectMacro("__FILE__", replacement);
    }
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

// 6.4.3p2: a UCN shall not specify a character whose short identifier is less
// than 00A0 (other than 0024 '$', 0040 '@', 0060 '`'), nor one in the
// surrogate range D800-DFFF.
static bool ucnDisallowedCodePoint(uint32_t cp) {
    if (cp < 0xA0) return cp != 0x24 && cp != 0x40 && cp != 0x60;
    return cp >= 0xD800 && cp <= 0xDFFF;
}

// Annex D.1: ranges of characters allowed in identifiers.
static bool ucnAllowedInIdentifier(uint32_t cp) {
    static const uint32_t ranges[][2] = {
        {0x00A8,0x00A8},{0x00AA,0x00AA},{0x00AD,0x00AD},{0x00AF,0x00AF},
        {0x00B2,0x00B5},{0x00B7,0x00BA},{0x00BC,0x00BE},{0x00C0,0x00D6},
        {0x00D8,0x00F6},{0x00F8,0x00FF},
        {0x0100,0x167F},{0x1681,0x180D},{0x180F,0x1FFF},
        {0x200B,0x200D},{0x202A,0x202E},{0x203F,0x2040},{0x2054,0x2054},
        {0x2060,0x206F},{0x2070,0x218F},{0x2460,0x24FF},{0x2776,0x2793},
        {0x2C00,0x2DFF},{0x2E80,0x2FFF},{0x3004,0x3007},{0x3008,0x3020},
        {0x3021,0x30FF},{0x3031,0xD7FF},{0xF900,0xFD3D},{0xFD40,0xFDCF},
        {0xFDF0,0xFE44},{0xFE47,0xFFFD},
        {0x10000,0x1FFFD},{0x20000,0x2FFFD},{0x30000,0x3FFFD},{0x40000,0x4FFFD},
        {0x50000,0x5FFFD},{0x60000,0x6FFFD},{0x70000,0x7FFFD},{0x80000,0x8FFFD},
        {0x90000,0x9FFFD},{0xA0000,0xAFFFD},{0xB0000,0xBFFFD},{0xC0000,0xCFFFD},
        {0xD0000,0xDFFFD},{0xE0000,0xEFFFD},
    };
    for (const auto& r : ranges)
        if (cp >= r[0] && cp <= r[1]) return true;
    return false;
}

// Annex D.2: ranges disallowed as the initial character of an identifier.
static bool ucnDisallowedInitial(uint32_t cp) {
    return (cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1DC0 && cp <= 0x1DFF)
        || (cp >= 0x20D0 && cp <= 0x20FF) || (cp >= 0xFE20 && cp <= 0xFE2F);
}

// 6.4.2.1p3 / 6.4.3p2: validate every UCN spelled in an identifier. The
// Tokenizer lexes \uXXXX/\UXXXXXXXX into identifier tokens without judging the
// designated character; this runs once per identifier surfacing to the parser
// (see the pushToken*WithConcat call sites) since the Tokenizer itself has no
// diagnostics channel.
void Preprocessor::checkIdentifierUCNs(const PPToken& tok) {
    const std::string& s = tok.lexeme;
    if (s.find('\\') == std::string::npos) return;
    auto hexVal = [](char c) -> uint32_t {
        if (c >= '0' && c <= '9') return (uint32_t)(c - '0');
        if (c >= 'a' && c <= 'f') return 10u + (uint32_t)(c - 'a');
        return 10u + (uint32_t)(c - 'A');
    };
    for (size_t i = 0; i < s.size();) {
        if (s[i] != '\\' || i + 1 >= s.size() || (s[i+1] != 'u' && s[i+1] != 'U')) { ++i; continue; }
        const size_t need = (s[i+1] == 'u') ? 4 : 8;
        if (i + 2 + need > s.size()) break;
        uint32_t cp = 0;
        bool valid = true;
        for (size_t k = 0; k < need; ++k) {
            char c = s[i + 2 + k];
            if (!isxdigit(static_cast<unsigned char>(c))) { valid = false; break; }
            cp = (cp << 4) + hexVal(c);
        }
        if (!valid) { ++i; continue; }
        const std::string spelled = s.substr(i, 2 + need);
        if (ucnDisallowedCodePoint(cp)) {
            diagnostics.push_back(Diagnostic{
                .message = "universal character name '" + spelled + "' names a disallowed code point (6.4.3p2)",
                .severity = Diagnostic::Severity::Error, .span = tok.span});
        } else if (!ucnAllowedInIdentifier(cp)) {
            diagnostics.push_back(Diagnostic{
                .message = "universal character name '" + spelled + "' designates a character not allowed in an identifier (6.4.2.1p3)",
                .severity = Diagnostic::Severity::Error, .span = tok.span});
        } else if (i == 0 && ucnDisallowedInitial(cp)) {
            diagnostics.push_back(Diagnostic{
                .message = "universal character name '" + spelled + "' designates a character not allowed as the initial character of an identifier (6.4.2.1p3)",
                .severity = Diagnostic::Severity::Error, .span = tok.span});
        }
        i += 2 + need;
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
    // Already normalized: the decoded metadata is only ever set by a completed
    // normalization (or by paths that produce pre-decoded tokens, e.g. phase-6
    // concatenation and stringification). Re-running would decode the already
    // decoded lexeme a second time and duplicate escape-range diagnostics.
    if (tok.decodedString.has_value() || tok.decodedCharValue.has_value()) return;
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

    // 6.4.4.4p9: an octal or hexadecimal escape must be representable in the
    // constant's corresponding type — unsigned char when unprefixed or u8,
    // char16_t for u, char32_t/wchar_t for U/L (32-bit here, so only hex-digit
    // overflow can exceed it).
    const uint32_t escapeLimit = (prefix.empty() || prefix == "u8") ? 0xFFu
                               : (prefix == "u")                    ? 0xFFFFu
                                                                    : 0xFFFFFFFFu;

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
            bool overflow = false;
            while (p < inner.size() && isxdigit(static_cast<unsigned char>(inner[p]))) {
                char ch = inner[p++];
                if (val > (0xFFFFFFFFu >> 4)) overflow = true;
                val = (val << 4) + (uint32_t)( (ch>='0'&&ch<='9') ? ch - '0' : (ch>='a'&&ch<='f') ? 10 + ch - 'a' : 10 + ch - 'A');
            }
            if (p == start) {
                diagnostics.push_back(Diagnostic{.message = "\'\\x\' escape with no hex digits", .severity = Diagnostic::Severity::Error, .span = tok.span});
            }
            if (overflow || val > escapeLimit) {
                diagnostics.push_back(Diagnostic{.message = "hexadecimal escape sequence out of range (6.4.4.4p9)", .severity = Diagnostic::Severity::Error, .span = tok.span});
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
            } else if (ucnDisallowedCodePoint(val)) {
                // 6.4.3p2 applies to UCNs in literals too: a UCN naming
                // 0041 is a constraint violation, not a spelling of 'A'.
                diagnostics.push_back(Diagnostic{.message = "universal character name names a disallowed code point (6.4.3p2)", .severity = Diagnostic::Severity::Error, .span = tok.span});
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
            if (val > escapeLimit) {
                diagnostics.push_back(Diagnostic{.message = "octal escape sequence out of range (6.4.4.4p9)", .severity = Diagnostic::Severity::Error, .span = tok.span});
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
        if (tok.kind == PPTokenKind::Identifier) checkIdentifierUCNs(tok);
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

    // No previous in-buffer literal to merge with. Append the (normalized)
    // literal. Phase-6 concatenation with *following* tokens is handled by
    // next(), which pulls and macro-expands subsequent tokens on demand — so a
    // macro after a string literal (`"%" PRIx8`) is expanded before the
    // concatenation decision. Doing tokenizer look-ahead here instead consumed
    // that macro identifier raw (unexpanded) and collided with the front-push
    // path used by macro expansion (issue #90 gap 1).
    PPToken merged = tok;
    normalizeLiteralToken(merged);
    outBuffer.push_back(merged);
}

void Preprocessor::pushTokenFrontWithConcat(const PPToken& tok) {
    // Only attempt special Phase-6 behavior for string literals; otherwise push normally.
    if (tok.kind != PPTokenKind::StringLiteral) {
        if (tok.kind == PPTokenKind::Identifier) checkIdentifierUCNs(tok);
        outBuffer.push_front(tok);
        return;
    }

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
    if (isPragmaOnceDirective(tokens)) return true;
    if (isStdcFpContractDirective(tokens)) return true;

    std::string pragmaContent = buildPragmaContent(tokens);
    trimTrailingWhitespace(pragmaContent);

    diagnostics.push_back(Diagnostic{
        .message = std::string("#pragma: ") + (pragmaContent.empty() ? "(empty)" : pragmaContent),
        .severity = Diagnostic::Severity::Warning,
        .span = tokens.empty() ? std::nullopt : std::optional<SourceSpan>(tokens[0].span)
    });

    return true;
}

// `#pragma STDC FP_CONTRACT ON|OFF|DEFAULT` (6.10.6p2, 6.5p8). Accepted with
// no effect: wvmcc never contracts floating expressions, so the state is
// permanently OFF (documented in docs/spec.md) and every setting is honored
// vacuously (ON merely *permits* contraction). Recognizing it here keeps the
// standard pragma from tripping the unknown-pragma warning (#113).
bool Preprocessor::isStdcFpContractDirective(const std::vector<PPToken>& tokens) {
    size_t idx = 0;
    auto skipWs = [&]{ while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) idx++; };
    auto ident = [&](std::initializer_list<const char*> names) -> bool {
        if (idx >= tokens.size() || tokens[idx].kind != PPTokenKind::Identifier) return false;
        for (const char* n : names) {
            if (tokens[idx].lexeme == n) { idx++; return true; }
        }
        return false;
    };
    skipWs();
    if (!ident({"STDC"})) return false;
    skipWs();
    if (!ident({"FP_CONTRACT"})) return false;
    skipWs();
    if (!ident({"ON", "OFF", "DEFAULT"})) return false;
    skipWs();
    return idx == tokens.size();
}

std::optional<PPToken> Preprocessor::pullPragmaOperandToken() {
    while (true) {
        if (!outBuffer.empty()) {
            PPToken t = outBuffer.front();
            outBuffer.pop_front();
            if (t.kind == PPTokenKind::Whitespace || t.kind == PPTokenKind::Newline) continue;
            return t;
        }
        auto opt = readRawToken();
        if (!opt) return std::nullopt;
        PPToken t = *opt;
        if (t.kind == PPTokenKind::Whitespace || t.kind == PPTokenKind::Newline) continue;
        if (t.kind == PPTokenKind::Identifier && macroTable.isDefined(t.lexeme)) {
            // Expansion results land at the front of outBuffer and are picked
            // up by the next iteration.
            if (handleIdentifierExpansionToken(t)) continue;
        }
        return t;
    }
}

std::string Preprocessor::destringizePragmaOperand(const std::string& lexeme) {
    // Strip an encoding prefix (L, u, U, u8) and the surrounding quotes.
    size_t begin = lexeme.find('"');
    size_t end = lexeme.rfind('"');
    if (begin == std::string::npos || end <= begin) return {};
    std::string out;
    out.reserve(end - begin);
    for (size_t i = begin + 1; i < end; ++i) {
        if (lexeme[i] == '\\' && i + 1 < end && (lexeme[i + 1] == '"' || lexeme[i + 1] == '\\')) {
            ++i;    // 6.10.9p2: only \" and \\ are undone
        }
        out.push_back(lexeme[i]);
    }
    return out;
}

void Preprocessor::handlePragmaOperator(const PPToken& opTok) {
    auto fail = [&](const char* msg) {
        diagnostics.push_back(Diagnostic{
            .message = msg,
            .severity = Diagnostic::Severity::Error,
            .span = opTok.span});
    };

    auto lp = pullPragmaOperandToken();
    if (!lp || lp->kind != PPTokenKind::Punctuator || lp->lexeme != "(") {
        fail("expected '(' after _Pragma operator");
        return;
    }
    auto str = pullPragmaOperandToken();
    if (!str || str->kind != PPTokenKind::StringLiteral) {
        fail("operand of _Pragma must be a parenthesized string literal");
        return;
    }
    auto rp = pullPragmaOperandToken();
    if (!rp || rp->kind != PPTokenKind::Punctuator || rp->lexeme != ")") {
        fail("expected ')' after _Pragma operand");
        return;
    }

    // 6.10.9p2: destringize the (raw, not escape-decoded) literal and process
    // the character sequence as the pp-tokens of a #pragma directive.
    std::string content = destringizePragmaOperand(str->lexeme);
    std::istringstream iss(content);
    Tokenizer contentTokenizer(iss);
    std::vector<PPToken> lineTokens;
    while (auto t = contentTokenizer.next()) {
        if (t->kind == PPTokenKind::Newline) break;
        // Tokens carry the operator's location: the content has no file of its own.
        t->span = opTok.span;
        lineTokens.push_back(*t);
    }
    handlePragmaDirective(lineTokens);
}

} // namespace wvmcc
