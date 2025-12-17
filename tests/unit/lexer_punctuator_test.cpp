#include <fstream>
#include <iostream>
#include <vector>

#include "../../src/pp/Preprocessor.hpp"
#include "../../src/parser/Lexer.hpp"

int main() {
    using namespace wvmcc;
    const std::string fname = "temp_lexer_punct.c";
    const std::vector<std::string> puncs = {
        ";", ",", "(", ")", "{", "}", "+", "-", "*", "/",
        "==", "!=", "->", "<=", ">=", "<", ">", "&&", "||", "=", "++", "--", "."
    };

    {
        std::ofstream ofs(fname);
        for (size_t i = 0; i < puncs.size(); ++i) {
            if (i) ofs << ' ';
            ofs << puncs[i];
        }
        ofs << '\n';
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); std::cerr << "failed to open input\n"; return 1; }
    wvmcc::parser::Lexer lex(pp);
    std::vector<wvmcc::parser::Token> toks;
    while (auto t = lex.next()) toks.push_back(*t);
    std::remove(fname.c_str());

    if (toks.size() != puncs.size()) {
        std::cerr << "Unexpected token count: " << toks.size() << " expected " << puncs.size() << "\n";
        return 2;
    }

    for (size_t i = 0; i < puncs.size(); ++i) {
        if (toks[i].kind() != wvmcc::parser::TokenKind::Punctuator) {
            std::cerr << "Token " << i << " kind mismatch: got " << (int)toks[i].kind() << " expected Punctuator\n";
            return 3;
        }
        if (toks[i].lexeme() != puncs[i]) {
            std::cerr << "Token " << i << " lexeme mismatch: got '" << toks[i].lexeme() << "' expected '" << puncs[i] << "'\n";
            return 4;
        }
    }

    std::cout << "lexer_punctuator_test: OK\n";
    return 0;
}
