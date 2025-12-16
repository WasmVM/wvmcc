#include "Preprocessor.hpp"

#include <fstream>
#include <sstream>
#include "Tokenizer.hpp"

namespace wvmcc {

PreprocessResult Preprocessor::run(const std::string& inputPath) const {
    std::ifstream ifs(inputPath);
    if (!ifs) {
        return PreprocessResult{std::vector<wvmcc::PPToken>{}, false, std::string("cannot open input: ") + inputPath};
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    // Single-pass tokenization (phase 1–3 handled inline)
    auto tokens = Tokenizer::tokenize(oss.str());
    // Validate tokens for errors (e.g., unterminated string literal)
    for (const auto& t : tokens) {
        if (t.kind == PPTokenKind::StringLiteral) {
            if (t.lexeme.empty() || t.lexeme.back() != '"') {
                std::ostringstream em;
                em << "unterminated string literal at line " << t.span.begin.line
                   << ", column " << t.span.begin.column;
                return PreprocessResult{std::vector<PPToken>{}, false, em.str()};
            }
        } else if (t.kind == PPTokenKind::CharConst) {
            if (t.lexeme.empty() || t.lexeme.back() != '\'') {
                std::ostringstream em;
                em << "unterminated character constant at line " << t.span.begin.line
                   << ", column " << t.span.begin.column;
                return PreprocessResult{std::vector<PPToken>{}, false, em.str()};
            }
        }
    }
    // Post-pass: recognize header-name tokens only within #include
    std::vector<PPToken> transformed;
    transformed.reserve(tokens.size());
    for (size_t i=0;i<tokens.size();) {
        const auto& t = tokens[i];
        if (t.kind == PPTokenKind::Punctuator && t.lexeme == "#") {
            size_t j = i + 1;
            while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) j++;
            // Leverage Identifier recognition for 'include'
            if (j < tokens.size() && tokens[j].kind == PPTokenKind::Identifier && tokens[j].lexeme == "include") {
                transformed.push_back(t);
                transformed.push_back(tokens[j]);
                j++;
                while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) j++;
                if (j < tokens.size()) {
                    if (tokens[j].kind == PPTokenKind::Punctuator && tokens[j].lexeme == "<") {
                        SourcePos begin = tokens[j].span.begin;
                        std::string acc;
                        size_t k = j + 1;
                        bool terminated = false;
                        while (k < tokens.size()) {
                            if (tokens[k].kind == PPTokenKind::Newline) break;
                            if (tokens[k].kind == PPTokenKind::Punctuator && tokens[k].lexeme == ">") { terminated = true; break; }
                            acc += tokens[k].lexeme;
                            k++;
                        }
                        if (terminated) {
                            SourcePos end = tokens[k].span.end;
                            transformed.push_back(PPToken{PPTokenKind::HeaderName, SourceSpan{begin, end}, std::string("<") + acc + ">"});
                            i = k + 1;
                            continue;
                        }
                    } else if (tokens[j].kind == PPTokenKind::StringLiteral) {
                        transformed.push_back(PPToken{PPTokenKind::HeaderName, tokens[j].span, tokens[j].lexeme});
                        i = j + 1;
                        continue;
                    }
                }
            }
        }
        transformed.push_back(t);
        i++;
    }

    return PreprocessResult{std::move(transformed), true, std::string()};
}


// phase1to3_process removed; preprocessing is centralized in Tokenizer::preprocess


} // namespace wvmcc
