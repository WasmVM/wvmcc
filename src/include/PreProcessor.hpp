#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <filesystem>
#include <stack>
#include <optional>
#include <Token.hpp>
#include <SourceFile.hpp>

namespace WasmVM {

struct PreProcessor {

    PreProcessor(std::filesystem::path path);

    struct TokenStream {
        TokenStream(std::filesystem::path path);
        std::optional<Token> get();
    private:
        SourceFile source;
    };

    std::optional<Token> get();

private:
    std::stack<TokenStream> streams;
};

} // namespace WasmVM

#endif