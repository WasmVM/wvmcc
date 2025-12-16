#include "Preprocessor.hpp"
#include "ConstExprParser.hpp"

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include "Tokenizer.hpp"

namespace wvmcc {

// Helper function to stringify tokens (for # operator in macros)
// Now a member method (forward reference removed)

static bool file_ends_with_newline(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return true; // if unreadable, avoid noisy warnings here
    f.seekg(0, std::ios::end);
    std::streampos sz = f.tellg();
    if (sz <= 0) return true; // empty file: treat as ok
    f.seekg(-1, std::ios::end);
    char last = '\n';
    f.read(&last, 1);
    return last == '\n';
}

// Minimal streaming API implementation (object-like macros, includes, simple conditionals)

bool Preprocessor::open(const std::string& inputPath) {
    return pushFile(inputPath);
}

std::optional<PPToken> Preprocessor::peek() {
    ensureBuffer();
    if (outBuffer.empty()) return std::nullopt;
    return outBuffer.front();
}

std::optional<PPToken> Preprocessor::next() {
    ensureBuffer();
    if (outBuffer.empty()) return std::nullopt;
    auto t = outBuffer.front(); outBuffer.pop_front();
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
    ctx.dir = std::filesystem::path(path).parent_path().string();
    // Track inclusion stack for streaming mode and define predefined macros
    namespace fs = std::filesystem;
    std::string canonicalPath = fs::weakly_canonical(fs::absolute(path)).string();
    if (!pushInclusion(canonicalPath)) {
        return false; // cycle detected
    }

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
        if (!tok) { fileStack.pop_back(); atLineStart = false; continue; }
        return tok;
    }
    return std::nullopt;
}

void Preprocessor::ensureBuffer() {
    while (outBuffer.empty()) {
        auto opt = readRawToken();
        if (!opt) return;
        auto tok = *opt;

        if (tok.kind == PPTokenKind::Newline) {
            outBuffer.push_back(tok);
            atLineStart = true;
            continue;
        }

        if (atLineStart && tok.kind == PPTokenKind::Punctuator && tok.lexeme == "#") {
            handleDirective();
            continue;
        }

        if (tok.kind == PPTokenKind::Identifier) {
            if (macroTable.isDefined(tok.lexeme)) {
                auto mOpt = macroTable.getMacro(tok.lexeme);
                if (mOpt) {
                    const Macro* m = *mOpt;
                    if (!m->isFunction) {
                        for (auto it = m->replacement.rbegin(); it != m->replacement.rend(); ++it) {
                            outBuffer.push_front(*it);
                        }
                        continue;
                    } else {
                        // Function-like macro: attempt to read the invocation (including parentheses)
                        // without emitting until expansion decision is made.
                        // Use the tokenizer from the current file context for lookahead/consumption.
                        if (!fileStack.empty()) {
                            auto &ctx = fileStack.back();
                            // Peek to find if '(' follows (skip whitespace)
                            std::optional<PPToken> p;
                            size_t lookIdx = 0;
                            // Use tokenizer peek to inspect next tokens without consuming
                            while ((p = ctx.tokenizer->peek())) {
                                if (p->kind == PPTokenKind::Whitespace) {
                                    // do not consume here, just check
                                    ctx.tokenizer->next();
                                    // record whitespace as part of invocation if needed
                                    // continue scanning
                                    continue;
                                }
                                break;
                            }
                            // After skipping whitespace via peek+next above, check current peek
                            auto nextTok = ctx.tokenizer->peek();
                            if (nextTok && nextTok->kind == PPTokenKind::Punctuator && nextTok->lexeme == "(") {
                                // We have a function-like invocation; consume tokens to collect full invocation
                                std::vector<PPToken> invocation;
                                invocation.push_back(tok); // identifier
                                int parenDepth = 0;
                                // consume tokens until matching ')' encountered
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
                                // Try expansion
                                size_t idx = 0;
                                std::vector<PPToken> expandedResult;
                                if (tryExpandFunctionLikeMacro(invocation, idx, m, tok, expandedResult)) {
                                    // Push expanded tokens into outBuffer
                                    for (const auto &et : expandedResult) outBuffer.push_back(et);
                                    continue; // processed, move to next token
                                } else {
                                    // Expansion failed; emit original invocation tokens
                                    for (const auto &itok : invocation) outBuffer.push_back(itok);
                                    continue;
                                }
                            }
                            // If no '(', fallthrough to emit identifier normally
                        }
                    }
                }
            }
        }

        outBuffer.push_back(tok);
        atLineStart = false;
    }
}

void Preprocessor::skipToEndOfLineTokensFromTokenizer(Tokenizer& tz) {
    while (auto t = tz.next()) { if (t->kind == PPTokenKind::Newline) break; }
}

void Preprocessor::handleDirective() {
    std::optional<PPToken> tok;
    tok = readRawToken();
    if (!tok) return;
    while (tok && tok->kind == PPTokenKind::Whitespace) tok = readRawToken();
    if (!tok) return;
    if (tok->kind != PPTokenKind::Identifier) {
        if (tok) skipLineFromToken(std::move(*tok));
        return;
    }
    std::string dir = tok->lexeme;
    if (dir == "define") {
        handleDefine();
    } else if (dir == "undef") {
        handleUndef();
    } else if (dir == "include") {
        handleInclude();
    } else if (dir == "ifdef") {
        handleIfdef(true);
    } else if (dir == "ifndef") {
        handleIfdef(false);
    } else {
        std::optional<PPToken> t;
        while ((t = readRawToken())) { if (t->kind == PPTokenKind::Newline) break; }
    }
}

void Preprocessor::skipLineFromToken(PPToken t) {
    if (t.kind == PPTokenKind::Newline) return;
    std::optional<PPToken> tn;
    while ((tn = readRawToken())) { if (tn->kind == PPTokenKind::Newline) break; }
}

void Preprocessor::handleDefine() {
    if (fileStack.empty()) return;
    auto &tz = *fileStack.back().tokenizer;
    // Skip whitespace
    while (auto p = tz.peek()) { if (p->kind == PPTokenKind::Whitespace) tz.next(); else break; }
    auto nameTok = tz.peek();
    if (!nameTok || nameTok->kind != PPTokenKind::Identifier) {
        // consume until end of line
        skipLineFromToken(nameTok ? *nameTok : PPToken{});
        return;
    }
    std::string name = nameTok->lexeme;
    tz.next(); // consume name

    bool isFunction = false;
    std::vector<std::string> params;
    bool variadic = false;

    auto next = tz.peek();
    if (next && next->kind == PPTokenKind::Punctuator && next->lexeme == "(") {
        isFunction = true;
        tz.next(); // consume '('
        if (!parseMacroParameters(tz, params, variadic)) {
            // error already emitted by parseMacroParameters
            return;
        }
    }

    std::vector<PPToken> replacement = collectLineTokens(tz);
    trimReplacementWhitespace(replacement);

    if (isFunction) {
        macroTable.defineFunctionMacro(name, params, replacement, variadic);
    } else {
        macroTable.defineObjectMacro(name, replacement);
    }
}

void Preprocessor::handleUndef() {
    auto tok = readRawToken();
    while (tok && tok->kind == PPTokenKind::Whitespace) tok = readRawToken();
    if (!tok || tok->kind != PPTokenKind::Identifier) { skipLineFromToken(tok ? *tok : PPToken{}); return; }
    macroTable.undefine(tok->lexeme);
    while (true) { auto t = readRawToken(); if (!t || t->kind == PPTokenKind::Newline) break; }
}

void Preprocessor::handleInclude() {
    auto tok = readRawToken();
    while (tok && tok->kind == PPTokenKind::Whitespace) tok = readRawToken();
    if (!tok) return;
    std::string header;
    if (tok->kind == PPTokenKind::HeaderName) {
        header = tok->lexeme;
        if (!header.empty() && (header.front() == '<' || header.front() == '"')) {
            if (header.front() == '<' && header.back() == '>') header = header.substr(1, header.size()-2);
            else if (header.front() == '"' && header.back() == '"') header = header.substr(1, header.size()-2);
        }
    } else if (tok->kind == PPTokenKind::StringLiteral) {
        auto s = tok->lexeme;
        if (!s.empty() && s.front() == '"' && s.back() == '"') s = s.substr(1, s.size()-2);
        header = s;
    } else {
        skipLineFromToken(*tok);
        return;
    }

    std::string resolved;
    if (!fileStack.empty()) {
        auto cand = std::filesystem::path(fileStack.back().path).parent_path() / header;
        if (std::filesystem::exists(cand)) resolved = cand.string();
    }
    if (resolved.empty()) {
        for (const auto &ip : includePaths) {
            auto cand = std::filesystem::path(ip) / header;
            if (std::filesystem::exists(cand)) { resolved = cand.string(); break; }
        }
    }
    if (!resolved.empty()) pushFile(resolved);
    while (true) { auto t = readRawToken(); if (!t || t->kind == PPTokenKind::Newline) break; }
}

void Preprocessor::handleIfdef(bool wantDefined) {
    auto tok = readRawToken();
    while (tok && tok->kind == PPTokenKind::Whitespace) tok = readRawToken();
    bool take = false;
    if (tok && tok->kind == PPTokenKind::Identifier) {
        bool def = macroTable.isDefined(tok->lexeme);
        take = wantDefined ? def : !def;
    }
    if (!take) {
        int depth = 1;
        while (auto t = readRawToken()) {
            if (t->kind == PPTokenKind::Punctuator && t->lexeme == "#") {
                auto nt = readRawToken();
                while (nt && nt->kind == PPTokenKind::Whitespace) nt = readRawToken();
                if (nt && nt->kind == PPTokenKind::Identifier) {
                    if (nt->lexeme == "ifdef" || nt->lexeme == "ifndef" || nt->lexeme == "if") depth++;
                    else if (nt->lexeme == "endif") { depth--; if (depth==0) break; }
                }
            }
        }
    }
}


bool Preprocessor::validateLiteralToken(const PPToken& t, std::string& errMsg) const {
    if (t.kind == PPTokenKind::StringLiteral) {
        if (t.lexeme.empty() || t.lexeme.back() != '"') {
            std::ostringstream em;
            em << "unterminated string literal at line " << t.span.begin.line
               << ", column " << t.span.begin.column;
            errMsg = em.str();
            return false;
        }
    } else if (t.kind == PPTokenKind::CharConst) {
        if (t.lexeme.empty() || t.lexeme.back() != '\'' ) {
            std::ostringstream em;
            em << "unterminated character constant at line " << t.span.begin.line
               << ", column " << t.span.begin.column;
            errMsg = em.str();
            return false;
        }
    }
    return true;
}

void Preprocessor::emitIfActive(const PPToken& t, std::vector<PPToken>& out) const {
    if (conditionalStack.empty() || conditionalStack.back().currentlyActive) {
        out.push_back(t);
    }
}

void Preprocessor::consumeToEndOfLine(Tokenizer& tokenizer) {
    while (auto a = tokenizer.peek()) { if (a->kind == PPTokenKind::Newline) break; tokenizer.next(); }
}

bool Preprocessor::handleDirectiveIfdef(Tokenizer& tokenizer, bool& hasErrors) {
    auto lineTokens = collectLineTokens(tokenizer);
    if (lineTokens.empty()) {
        diagnostics.push_back(Diagnostic{ .message = "expected macro name after #ifdef", .severity = Diagnostic::Severity::Error, .span = std::nullopt });
        return true;
    }
    std::string macroName;
    for (const auto& lt : lineTokens) { if (lt.kind == PPTokenKind::Identifier) { macroName = lt.lexeme; break; } }
    if (macroName.empty()) {
        diagnostics.push_back(Diagnostic{ .message = "expected macro name after #ifdef", .severity = Diagnostic::Severity::Error, .span = std::nullopt });
        return true;
    }
    if (!handleIfdefDirective(macroName)) hasErrors = true;
    return true;
}

bool Preprocessor::handleDirectiveIfndef(Tokenizer& tokenizer, bool& hasErrors) {
    auto lineTokens = collectLineTokens(tokenizer);
    if (lineTokens.empty()) {
        diagnostics.push_back(Diagnostic{ .message = "expected macro name after #ifndef", .severity = Diagnostic::Severity::Error, .span = std::nullopt });
        return true;
    }
    std::string macroName;
    for (const auto& lt : lineTokens) { if (lt.kind == PPTokenKind::Identifier) { macroName = lt.lexeme; break; } }
    if (macroName.empty()) {
        diagnostics.push_back(Diagnostic{ .message = "expected macro name after #ifndef", .severity = Diagnostic::Severity::Error, .span = std::nullopt });
        return true;
    }
    if (!handleIfndefDirective(macroName)) hasErrors = true;
    return true;
}

bool Preprocessor::handleDirectiveConditional(Tokenizer& tokenizer, const std::string& dirLexeme, bool& hasErrors) {
    auto lineTokens = collectLineTokens(tokenizer);
    if (dirLexeme == "if") {
        if (!handleIfDirective(tokenizer, lineTokens)) hasErrors = true;
    } else if (dirLexeme == "elif") {
        if (!handleElifDirective(tokenizer, lineTokens)) hasErrors = true;
    }
    return true;
}

bool Preprocessor::handleDirectiveMessage(Tokenizer& tokenizer, const std::string& dirLexeme, bool& hasErrors) {
    auto lt = collectLineTokens(tokenizer);
    if (dirLexeme == "error") {
        if (!handleErrorDirective(lt)) hasErrors = true;
    } else if (dirLexeme == "warning") {
        if (!handleWarningDirective(lt)) hasErrors = true;
    }
    return true;
}

bool Preprocessor::handleDirectiveInclude(Tokenizer& tokenizer, const PPToken& hashTok, 
                                          const PPToken& dirToken, std::vector<PPToken>& out,
                                          const std::string& currentDir, bool& hasErrors) {
    bool active = conditionalStack.empty() || conditionalStack.back().currentlyActive;
    bool executed = active && handleIncludeDirective(tokenizer, out, currentDir);
    if (executed) return true;
    if (active) { hasErrors = true; return true; }
    bool hasErr = false;
    for (const auto& d : diagnostics) { if (d.severity == Diagnostic::Severity::Error) { hasErr = true; break; } }
    if (hasErr) { hasErrors = true; } else { out.push_back(hashTok); out.push_back(dirToken); }
    return true;
}

bool Preprocessor::handleDirectiveDefineUndef(Tokenizer& tokenizer, const std::string& dirLexeme, bool& hasErrors) {
    bool active = conditionalStack.empty() || conditionalStack.back().currentlyActive;
    if (dirLexeme == "define") {
        if (active && !handleDefineDirective(tokenizer)) hasErrors = true;
        if (!active) consumeToEndOfLine(tokenizer);
    } else if (dirLexeme == "undef") {
        if (active && !handleUndefDirective(tokenizer)) hasErrors = true;
        if (!active) consumeToEndOfLine(tokenizer);
    }
    return true;
}

bool Preprocessor::handleDirectiveLine(Tokenizer& tokenizer, bool& hasErrors) {
    bool active = conditionalStack.empty() || conditionalStack.back().currentlyActive;
    if (active) { auto lt = collectLineTokens(tokenizer); if (!handleLineDirective(lt)) hasErrors = true; }
    else { consumeToEndOfLine(tokenizer); }
    return true;
}

bool Preprocessor::handleDirectivePragma(Tokenizer& tokenizer, bool& hasErrors) {
    bool active = conditionalStack.empty() || conditionalStack.back().currentlyActive;
    if (active) { auto lt = collectLineTokens(tokenizer); if (!handlePragmaDirective(lt)) hasErrors = true; }
    else { consumeToEndOfLine(tokenizer); }
    return true;
}

bool Preprocessor::handleDirectiveConditionalsGroup(Tokenizer& tokenizer, const std::string& dirLexeme, bool& hasErrors) {
    if (dirLexeme == "if" || dirLexeme == "elif") return handleDirectiveConditional(tokenizer, dirLexeme, hasErrors);
    if (dirLexeme == "ifdef") return handleDirectiveIfdef(tokenizer, hasErrors);
    if (dirLexeme == "ifndef") return handleDirectiveIfndef(tokenizer, hasErrors);
    if (dirLexeme == "else") { if (!handleElseDirective()) hasErrors = true; return true; }
    if (dirLexeme == "endif") { if (!handleEndifDirective()) hasErrors = true; return true; }
    return false;
}

void Preprocessor::skipDirectiveWhitespace(Tokenizer& tokenizer) {
    while (auto p = tokenizer.peek()) {
        if (p->kind != PPTokenKind::Whitespace) break;
        tokenizer.next();
    }
}

bool Preprocessor::checkActiveState() {
    return conditionalStack.empty() || conditionalStack.back().currentlyActive;
}

void Preprocessor::consumeUnknownDirective(Tokenizer& tokenizer, std::vector<PPToken>& out) {
    while (auto a = tokenizer.peek()) {
        if (a->kind == PPTokenKind::Newline) break;
        out.push_back(*tokenizer.next());
    }
}

bool Preprocessor::routeDirective(Tokenizer& tokenizer, const PPToken& hashTok, const PPToken& dir,
                                  std::vector<PPToken>& out, const std::string& currentDir, bool& hasErrors) {
    const std::string& dirName = dir.lexeme;
    bool active = checkActiveState();

    if (dirName == "include") return handleDirectiveInclude(tokenizer, hashTok, dir, out, currentDir, hasErrors);
    if (dirName == "define" || dirName == "undef") return handleDirectiveDefineUndef(tokenizer, dirName, hasErrors);
    if (handleDirectiveConditionalsGroup(tokenizer, dirName, hasErrors)) return true;
    if (dirName == "error" || dirName == "warning") {
        if (!active) { consumeToEndOfLine(tokenizer); return true; }
        return handleDirectiveMessage(tokenizer, dirName, hasErrors);
    }
    if (dirName == "line") return handleDirectiveLine(tokenizer, hasErrors);
    if (dirName == "pragma") return handleDirectivePragma(tokenizer, hasErrors);
    return false;
}

bool Preprocessor::processDirective(Tokenizer& tokenizer,
                                    const PPToken& hashTok,
                                    std::vector<PPToken>& out,
                                    const std::string& currentDir,
                                    bool& hasErrors) {
    skipDirectiveWhitespace(tokenizer);
    auto dir = tokenizer.peek();
    if (!dir || dir->kind != PPTokenKind::Identifier) return false;

    tokenizer.next();
    
    if (routeDirective(tokenizer, hashTok, *dir, out, currentDir, hasErrors)) {
        return true;
    }
    
    consumeUnknownDirective(tokenizer, out);
    return true;
}

bool Preprocessor::processLineStartToken(Tokenizer& tokenizer,
                                         const PPToken& t,
                                         std::vector<PPToken>& out,
                                         const std::string& currentDir,
                                         bool& hasErrors,
                                         bool& atLineStart) {
    if (t.kind == PPTokenKind::Whitespace) {
        out.push_back(t);
        return true; // remain at line start until non-whitespace
    }
    if (t.kind == PPTokenKind::Punctuator && t.lexeme == "#") {
        bool handled = processDirective(tokenizer, t, out, currentDir, hasErrors);
        atLineStart = false;
        return handled; // handled directive or unknown
    }
    emitIfActive(t, out);
    atLineStart = false;
    return true;
}

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
        child.inclusionStack = inclusionStack;
        child.pragmaOnceFiles = pragmaOnceFiles;
        auto childRes = child.run(*resolved);
        diagnostics.insert(diagnostics.end(), child.diagnostics.begin(), child.diagnostics.end());
        
        for (const auto& file : child.pragmaOnceFiles) {
            pragmaOnceFiles.insert(file);
        }
        
        if (childRes.success) {
            for (const auto& tk : childRes.tokens) out.push_back(tk);
            return true;
        }
        diagnostics.push_back(Diagnostic{
            .message = std::string("failed to read include file '") + *resolved + "'",
            .severity = Diagnostic::Severity::Error,
            .span = span
        });
        return false;
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

    // Both forms: search -I paths
    for (const auto& base : includePaths) {
        fs::path p = fs::path(base) / header;
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
    if (!inclusionStack.empty()) inclusionStack.pop_back();
}

PreprocessResult Preprocessor::run(const std::string& inputPath) {
    std::ifstream ifs(inputPath);
    if (!ifs) {
        return PreprocessResult{std::vector<wvmcc::PPToken>{}, false, std::string("cannot open input: ") + inputPath};
    }
    
    // Canonicalize path and check for cycles
    namespace fs = std::filesystem;
    std::string canonicalPath = fs::weakly_canonical(fs::absolute(inputPath)).string();
    if (!pushInclusion(canonicalPath)) {
        // Cycle detected
        diagnostics.push_back(Diagnostic{
            .message = std::string("cyclic include detected: ") + canonicalPath,
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return PreprocessResult{std::vector<wvmcc::PPToken>{}, false, std::string("cyclic include: ") + canonicalPath};
    }
    
    // Initialize predefined macros for this file (only once per run)
    if (inclusionStack.size() == 1) {
        // __FILE__ macro - current source file
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::StringLiteral,
                .span = {},
                .lexeme = "\"" + canonicalPath + "\"",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__FILE__", replacement);
        }
        
        // __LINE__ macro is handled dynamically during expansion (not predefined)
        
        // __DATE__ macro - compilation date
        {
            auto now = std::time(nullptr);
            auto tm = *std::localtime(&now);
            char dateStr[32];
            std::strftime(dateStr, sizeof(dateStr), "%b %d %Y", &tm);
            
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::StringLiteral,
                .span = {},
                .lexeme = "\"" + std::string(dateStr) + "\"",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__DATE__", replacement);
        }
        
        // __TIME__ macro - compilation time
        {
            auto now = std::time(nullptr);
            auto tm = *std::localtime(&now);
            char timeStr[32];
            std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
            
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::StringLiteral,
                .span = {},
                .lexeme = "\"" + std::string(timeStr) + "\"",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__TIME__", replacement);
        }
        
        // __STDC__ macro - ISO C compliance (set to 1)
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "1",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC__", replacement);
        }
        
        // __STDC_VERSION__ macro - C standard version (C17 = 201710L)
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "201710L",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_VERSION__", replacement);
        }
        
        // __STDC_HOSTED__ macro - freestanding implementation (0 = freestanding, 1 = hosted)
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "0",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_HOSTED__", replacement);
        }
        
        // __STDC_NO_ATOMICS__ macro - no atomic support
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "1",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_NO_ATOMICS__", replacement);
        }
        
        // __STDC_NO_COMPLEX__ macro - no complex number support
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "1",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_NO_COMPLEX__", replacement);
        }
        
        // __STDC_NO_THREADS__ macro - no threading support
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "1",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_NO_THREADS__", replacement);
        }
    }
    
    // Cleanup: pop inclusion on exit
    auto popGuard = [this]() { popInclusion(); };
    // Simplified: use a lambda immediately after scope, or just call at the end
    
    const bool endsWithNewline = file_ends_with_newline(inputPath);
    // Derive current file directory for quote-style includes
    std::filesystem::path inputPathFs(inputPath);
    std::string currentDir = inputPathFs.has_parent_path() ? inputPathFs.parent_path().string() : std::string();
    // Single-pass streaming: consume tokens, parse directives and includes
    Tokenizer tokenizer(ifs);
    std::vector<PPToken> out;
    out.reserve(256);
    tokenizer.reset();
    bool atLineStart = true; // start of file is line start
    bool hasErrors = false;  // Track errors during preprocessing
    while (auto tokOpt = tokenizer.next()) {
        PPToken t = *tokOpt;
        std::string errMsg;
        if (!validateLiteralToken(t, errMsg)) {
            popInclusion();
            return PreprocessResult{std::vector<PPToken>{}, false, errMsg};
        }

        if (t.kind == PPTokenKind::Newline) {
            out.push_back(t);
            atLineStart = true;
            continue;
        }

        if (!processLineStartToken(tokenizer, t, out, currentDir, hasErrors, atLineStart)) {
            // If not handled as a line-start special case, emit normally if active
            emitIfActive(t, out);
            atLineStart = false;
        }
    }

    if (!endsWithNewline) {
        diagnostics.push_back(Diagnostic{
            .message = std::string("input file '") + inputPath + "' does not end with a newline",
            .severity = Diagnostic::Severity::Warning,
            .span = std::nullopt
        });
    }
    
    // Check for unclosed conditional directives
    if (!conditionalStack.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#if/#ifdef/#ifndef without matching #endif",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        hasErrors = true;
    }
    
    // TODO: Include guard detection will be implemented properly
    // For now, the optimization is disabled to avoid incorrect behavior
    
    popInclusion();
    
    // Return failure if any errors were encountered during preprocessing
    if (hasErrors) {
        return PreprocessResult{std::vector<PPToken>{}, false, std::string("preprocessing failed with errors")};
    }

    // Optional debug path: skip macro expansion when PP_DEBUG_NO_EXPAND is set.
    if (std::getenv("PP_DEBUG_NO_EXPAND")) {
        return PreprocessResult{out, true, std::string()};
    }

    // Expand macros in the token stream
    std::vector<PPToken> expanded = expandMacros(out);

    return PreprocessResult{std::move(expanded), true, std::string()};
}

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
                                    const PPToken& tok, int& parenDepth, size_t& endIdx, size_t k) {
    parenDepth--;
    if (parenDepth == 0) {
        if (!currentArg.empty() || args.empty()) {
            args.push_back(currentArg);
        }
        endIdx = k;
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
            if (handleCloseParen(currentArg, args, tok, parenDepth, endIdx, k)) {
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
    frame.currentlyActive = frame.seenTrueBranch;
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
    if (std::getenv("PP_DEBUG_IFNDEF")) {
        std::cerr << "[pp] #ifndef " << macroName << " defined=" << (macroTable.isDefined(macroName) ? "1" : "0")
                  << " macros=" << macroTable.count() << "\n";
    }
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
    frame.currentlyActive = newActive;
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
    frame.currentlyActive = !frame.seenTrueBranch;
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
