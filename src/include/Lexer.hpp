#ifndef WVMCC_Lexer_DEF
#define WVMCC_Lexer_DEF

#include <PreProcessor.hpp>
#include <deque>

namespace WasmVM {

struct Lexer {
    Lexer(PreProcessor &pp);
    
    std::optional<Token> get();

private:
    PreProcessor &pp;
    std::deque<Token> buffer;
    std::optional<Token> next();
};

} // namespace WasmVM

#endif