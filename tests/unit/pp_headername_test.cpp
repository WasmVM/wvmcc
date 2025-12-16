#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
#include <unistd.h>

#include "../../src/pp/Preprocessor.hpp"
#include "../../src/pp/Tokenizer.hpp"
#include "../include/test_utils.hpp"

int main() {
    using K = wvmcc::PPTokenKind;
    using testutil::expectKindsLex;

    auto writeTemp = [](const std::string& content) -> std::string {
        char tpl[] = "/tmp/wvmcc_hdr_XXXXXX";
        int fd = mkstemp(tpl);
        if (fd == -1) return std::string();
        std::string path(tpl);
        std::ofstream ofs(path);
        ofs << content;
        ofs.close();
        close(fd);
        return path;
    };

    bool all_ok = true;

    // Angle-bracket header
    {
        std::string path = writeTemp("#include <stdio.h>\n");
        if (path.empty()) { std::cerr << "temp file error" << std::endl; return 1; }
        wvmcc::Preprocessor pp;
        auto res = pp.run(path);
        if (!res.success) { std::cerr << res.errorMsg << std::endl; return 1; }
        const auto& toks = res.tokens;
        // Expect: '#' 'include' HeaderName Newline
        if (toks.size() < 4 || toks[0].kind != K::Punctuator || toks[0].lexeme != "#"
            || toks[1].kind != K::Identifier || toks[1].lexeme != "include"
            || toks[2].kind != K::HeaderName || toks[2].lexeme != "<stdio.h>"
            || toks[3].kind != K::Newline) {
            std::cerr << "Angle-bracket header test failed" << std::endl;
            for (size_t i=0;i<toks.size();++i) {
                std::cerr << "["<<i<<"] kind="<<(int)toks[i].kind<<" lexeme='"<<toks[i].lexeme<<"'\n";
            }
            return 1;
        }
        std::remove(path.c_str());
    }

    // Quote header
    {
        std::string path = writeTemp("#include \"my/lib.h\"\n");
        if (path.empty()) { std::cerr << "temp file error" << std::endl; return 1; }
        wvmcc::Preprocessor pp;
        auto res = pp.run(path);
        if (!res.success) { std::cerr << res.errorMsg << std::endl; return 1; }
        const auto& toks = res.tokens;
        if (toks.size() < 4 || toks[0].kind != K::Punctuator || toks[0].lexeme != "#"
            || toks[1].kind != K::Identifier || toks[1].lexeme != "include"
            || toks[2].kind != K::HeaderName || toks[2].lexeme != "\"my/lib.h\""
            || toks[3].kind != K::Newline) {
            std::cerr << "Quote header test failed" << std::endl;
            for (size_t i=0;i+toks.size();++i) {
                std::cerr << "["<<i<<"] kind="<<(int)toks[i].kind<<" lexeme='"<<toks[i].lexeme<<"'\n";
            }
            return 1;
        }
        std::remove(path.c_str());
    }

    std::cout << "pp_headername_test: all cases passed" << std::endl;
    return 0;
}
