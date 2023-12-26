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
        enum class LineState : int{
            unknown = 0,
            normal = 1,
            hashed = 2,
            include = 3,
        } state;
    };

    std::optional<Token> get();
    bool is_text = false;

private:
    std::stack<TokenStream> streams;
    void skip_whitespace(std::optional<Token>& token);
    void if_directive(std::optional<Token>& token);
    void error_directive(std::optional<Token>& token);
};

} // namespace WasmVM

#endif