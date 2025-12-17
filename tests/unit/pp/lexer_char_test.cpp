#include <fstream>
#include <iostream>
#include <vector>

#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"

int main() {
    using namespace wvmcc;
    using namespace wvmcc::parser;
    const std::string fname = "temp_lexer_char.c";

    // character lexemes to test (lexeme, expected value, expected resolved type)
    const std::vector<std::tuple<std::string, std::uint32_t, CharacterInfo::ResolvedType>> cases = {
        {R"('x')", (std::uint32_t)'x', CharacterInfo::ResolvedType::UChar},
        {R"('\n')", (std::uint32_t)'\n', CharacterInfo::ResolvedType::UChar},
        {R"('\\')", (std::uint32_t)'\\', CharacterInfo::ResolvedType::UChar},
        {R"('\x41')", 0x41u, CharacterInfo::ResolvedType::UChar},
        {R"('\101')", 0101u, CharacterInfo::ResolvedType::UChar},
        {R"('ab')", (std::uint32_t)((('a'<<8) | 'b')), CharacterInfo::ResolvedType::UChar},
        {R"(L'a')", (std::uint32_t)'a', CharacterInfo::ResolvedType::WChar},
        {R"(u'a')", (std::uint32_t)'a', CharacterInfo::ResolvedType::Char16},
        {R"(U'a')", (std::uint32_t)'a', CharacterInfo::ResolvedType::Char32}
    };

    {
        std::ofstream ofs(fname);
        for (size_t i = 0; i < cases.size(); ++i) {
            if (i) ofs << ' ';
            ofs << std::get<0>(cases[i]);
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
        for (size_t i = 0; i < toks.size(); ++i) {
            std::string lex;
            TokenKind k = toks[i].kind();
            std::visit([&](auto&& tok){ using T = std::decay_t<decltype(tok)>; if constexpr(std::is_same_v<T, CharacterToken>) lex = tok.info.lexeme; else if constexpr(std::is_same_v<T, PunctuatorToken>) lex = tok.lexeme; else if constexpr(std::is_same_v<T, IdentifierToken>) lex = tok.name; else if constexpr(std::is_same_v<T, IntegerToken>) lex = tok.info.lexeme; else if constexpr(std::is_same_v<T, FloatingToken>) lex = tok.lexeme; else lex = "<other>"; }, toks[i].v);
            std::cerr << "  token["<<i<<"] kind="<<(int)k<<" lex='"<<lex<<"'\n";
        }
        return 2;
    }

    for (size_t i = 0; i < cases.size(); ++i) {
        const auto& [expectedLex, expectedVal, expectedResolved] = cases[i];
        if (toks[i].kind() != TokenKind::CharacterConstant) {
            std::cerr << "Token " << i << " kind mismatch: got " << (int)toks[i].kind() << " expected CharacterConstant\n";
            return 3;
        }

        bool ok = false;
        std::visit([&](auto&& tok){
            using T = std::decay_t<decltype(tok)>;
            if constexpr (std::is_same_v<T, CharacterToken>) {
                const auto& info = tok.info;
                if (info.lexeme != expectedLex) { std::cerr << "Token "<<i<<" lexeme mismatch: got '"<<info.lexeme<<"' expected '"<<expectedLex<<"'\n"; return; }
                if (info.value != expectedVal) { std::cerr << "Token "<<i<<" value mismatch: got "<<info.value<<" expected "<<expectedVal<<"\n"; return; }
                if (info.resolved != expectedResolved) { std::cerr << "Token "<<i<<" resolved mismatch\n"; return; }
                ok = true;
            }
        }, toks[i].v);

        if (!ok) return 4;
    }

    std::cout << "lexer_char_test: OK\n";
    return 0;
}
