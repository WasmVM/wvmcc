#include <fstream>
#include <iostream>
#include <vector>
#include <tuple>

#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"

int main() {
    using namespace wvmcc;
    using namespace wvmcc::parser;
    const std::string fname = "temp_lexer_float.c";

    // lexeme, expected resolved type
    const std::vector<std::pair<std::string, FloatingToken::ResolvedType>> cases = {
        {"1.0", FloatingToken::ResolvedType::Double},
        {"1.0f", FloatingToken::ResolvedType::Float},
        {"1.0F", FloatingToken::ResolvedType::Float},
        {"1e10", FloatingToken::ResolvedType::Double},
        {"1E-5", FloatingToken::ResolvedType::Double},
        {"0x1.8p+1", FloatingToken::ResolvedType::Double},
        {"0x1.8P+1L", FloatingToken::ResolvedType::LongDouble},
        {"123.", FloatingToken::ResolvedType::Double},
        {".5f", FloatingToken::ResolvedType::Float}
    };

    {
        std::ofstream ofs(fname);
        for (size_t i = 0; i < cases.size(); ++i) {
            if (i) ofs << ' ';
            ofs << cases[i].first;
        }
        ofs << '\n';
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); std::cerr << "failed to open input\n"; return 1; }
    wvmcc::parser::Lexer lex(pp);
    std::vector<wvmcc::parser::Token> toks;
    while (auto t = lex.next()) toks.push_back(*t);
    std::remove(fname.c_str());

    if (toks.size() != cases.size()) {
        std::cerr << "Unexpected token count: " << toks.size() << " expected " << cases.size() << "\n";
        return 2;
    }

    for (size_t i = 0; i < cases.size(); ++i) {
        const auto& [lexeme, expected] = cases[i];
        if (toks[i].kind() != TokenKind::FloatingConstant) {
            std::cerr << "Token " << i << " kind mismatch: got " << (int)toks[i].kind() << " expected FloatingConstant\n";
            return 3;
        }

        bool ok = false;
        std::visit([&](auto&& tok){
            using T = std::decay_t<decltype(tok)>;
            if constexpr (std::is_same_v<T, FloatingToken>) {
                if (tok.lexeme != lexeme) { std::cerr << "Token "<<i<<" lexeme mismatch: got '"<<tok.lexeme<<"' expected '"<<lexeme<<"'\n"; return; }
                if (tok.resolved != expected) { std::cerr << "Token "<<i<<" resolved mismatch\n"; return; }
                ok = true;
            }
        }, toks[i].v);

        if (!ok) return 4;
    }

    std::cout << "lexer_floating_test: OK\n";
    return 0;
}
