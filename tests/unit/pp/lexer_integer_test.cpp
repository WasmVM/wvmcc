#include <fstream>
#include <iostream>
#include <vector>
#include <tuple>

#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"

int main() {
    using namespace wvmcc;
    using namespace wvmcc::parser;
    const std::string fname = "temp_lexer_int.c";

    // lexeme, base (0=dec,1=oct,2=hex), value, isUnsigned, longCount, expected resolved type
    const std::vector<std::tuple<std::string,int,std::uint64_t,bool,int,IntegerInfo::ResolvedType>> cases = {
        {"0", 0, 0ULL, false, 0, IntegerInfo::ResolvedType::Int},
        {"123", 0, 123ULL, false, 0, IntegerInfo::ResolvedType::Int},
        {"0123", 1, 83ULL, false, 0, IntegerInfo::ResolvedType::Int},
        {"0x1A", 2, 0x1AULL, false, 0, IntegerInfo::ResolvedType::Int},
        {"0XFFu", 2, 0xFFULL, true, 0, IntegerInfo::ResolvedType::UnsignedInt},
        {"0777u", 1, 0777ULL, true, 0, IntegerInfo::ResolvedType::UnsignedInt},
        {"42ul", 0, 42ULL, true, 1, IntegerInfo::ResolvedType::UnsignedLong},
        {"42LLU", 0, 42ULL, true, 2, IntegerInfo::ResolvedType::UnsignedLongLong},
        {"0x10LL", 2, 0x10ULL, false, 2, IntegerInfo::ResolvedType::LongLong},
        {"0x10LLU", 2, 0x10ULL, true, 2, IntegerInfo::ResolvedType::UnsignedLongLong}
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
        return 2;
    }

    for (size_t i = 0; i < cases.size(); ++i) {
        const auto& [lex, base, val, isU, lc, expectedResolved] = cases[i];
        if (toks[i].kind() != TokenKind::IntegerConstant) {
            std::cerr << "Token " << i << " kind mismatch: got " << (int)toks[i].kind() << " expected IntegerConstant\n";
            return 3;
        }

        bool ok = false;
        std::visit([&](auto&& tok){
            using T = std::decay_t<decltype(tok)>;
            if constexpr (std::is_same_v<T, IntegerToken>) {
                const auto& info = tok.info;
                int gotBase = (info.base == IntegerInfo::Base::Decimal) ? 0 : (info.base == IntegerInfo::Base::Octal) ? 1 : 2;
                if (gotBase != base) { std::cerr << "Token "<<i<<" base mismatch: got "<<gotBase<<" expected "<<base<<"\n"; return; }
                if (info.value != val) { std::cerr << "Token "<<i<<" value mismatch: got "<<info.value<<" expected "<<val<<"\n"; return; }
                bool gotU = (info.flags & IntegerInfo::FLAG_UNSIGNED) != 0;
                if (gotU != isU) { std::cerr << "Token "<<i<<" unsigned mismatch\n"; return; }
                int gotLc = 0;
                if (info.flags & IntegerInfo::FLAG_LONG_LONG) gotLc = 2;
                else if (info.flags & IntegerInfo::FLAG_LONG) gotLc = 1;
                if (gotLc != lc) { std::cerr << "Token "<<i<<" longCount mismatch: got "<<gotLc<<" expected "<<lc<<"\n"; return; }
                if (info.lexeme != lex) { std::cerr << "Token "<<i<<" lexeme mismatch: got '"<<info.lexeme<<"' expected '"<<lex<<"'\n"; return; }
                if (info.resolved != expectedResolved) { std::cerr << "Token "<<i<<" resolved type mismatch\n"; return; }
                ok = true;
            }
        }, toks[i].v);

        if (!ok) return 4;
    }

    std::cout << "lexer_integer_test: OK\n";
    return 0;
}
