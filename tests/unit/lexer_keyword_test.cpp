#include <fstream>
#include <iostream>
#include <vector>

#include "../../src/pp/Preprocessor.hpp"
#include "../../src/parser/Lexer.hpp"

int main() {
    using namespace wvmcc;
    const std::string fname = "temp_lexer_kw.c";
    const std::vector<std::string> keywords = {
        "auto","extern","short","while","break","float","signed",
        "_Alignas","case","for","sizeof","_Alignof","char","goto",
        "static","_Atomic","const","if","struct","_Bool","continue",
        "inline","switch","_Complex","default","int","typedef",
        "_Generic","do","long","union","_Imaginary","double","register",
        "unsigned","_Noreturn","else","restrict","void","_Static_assert",
        "enum","return","volatile","_Thread_local"
    };

    {
        std::ofstream ofs(fname);
        for (size_t i = 0; i < keywords.size(); ++i) {
            if (i) ofs << ' ';
            ofs << keywords[i];
        }
        ofs << '\n';
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); std::cerr << "failed to open input\n"; return 1; }
    wvmcc::parser::Lexer lex(pp);
    std::vector<wvmcc::parser::Token> toks;
    while (auto t = lex.next()) toks.push_back(*t);
    std::remove(fname.c_str());

    if (toks.size() != keywords.size()) {
        std::cerr << "Unexpected token count: " << toks.size() << " expected " << keywords.size() << "\n";
        return 2;
    }

    for (size_t i = 0; i < keywords.size(); ++i) {
        if (toks[i].kind() != wvmcc::parser::TokenKind::Keyword) {
            std::cerr << "Token " << i << " kind mismatch: got " << (int)toks[i].kind() << " expected Keyword\n";
            return 3;
        }
        if (toks[i].lexeme() != keywords[i]) {
            std::cerr << "Token " << i << " lexeme mismatch: got '" << toks[i].lexeme() << "' expected '" << keywords[i] << "'\n";
            return 4;
        }
    }

    std::cout << "lexer_keyword_test: OK\n";
    return 0;
}
