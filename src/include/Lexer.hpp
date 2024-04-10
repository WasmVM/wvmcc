#ifndef WVMCC_Lexer_DEF
#define WVMCC_Lexer_DEF

#include <PreProcessor.hpp>

namespace WasmVM {

struct Lexer {
    Lexer(PreProcessor &pp);
    
    std::optional<Token> get();

private:
    PreProcessor &pp;
};

} // namespace WasmVM

#endif