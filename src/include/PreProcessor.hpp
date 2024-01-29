#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <filesystem>
#include <stack>
#include <string>
#include <vector>
#include <utility>
#include <iterator>
#include <optional>
#include <Token.hpp>
#include <SourceFile.hpp>

namespace WasmVM {

struct PreProcessor {

    PreProcessor(std::filesystem::path path);

    struct TokenStream {
        TokenStream(std::filesystem::path path);
        std::optional<Token> get();
        std::deque<Token> buffer;
    private:
        SourceFile source;
        enum class LineState : int{
            unknown = 0,
            normal = 1,
            hashed = 2,
            include = 3,
        } state;
    };

    struct Macro {
        std::string name;
        std::vector<std::string> params;
        std::vector<Token> replacement;
    };

    std::optional<Token> get();
    std::vector<Macro> macros;

private:
    bool is_text = false;
    std::stack<TokenStream> streams;

    void skip_whitespace(std::optional<Token>& token);
    std::optional<Token>& replace_macro(std::optional<Token>& token, std::deque<Token>& buffer);

    void if_directive(std::optional<Token>& token); // TODO:
    void elif_directive(std::optional<Token>& token); // TODO:
    void else_directive(std::optional<Token>& token); // TODO:
    void endif_directive(std::optional<Token>& token); // TODO:
    void define_directive(std::optional<Token>& token);
    void undef_directive(std::optional<Token>& token); // TODO:
    void pragma_directive(std::optional<Token>& token); // TODO:
    void include_directive(std::optional<Token>& token); // TODO:
    void line_directive(std::optional<Token>& token); // TODO:
    void error_directive(std::optional<Token>& token);
};

} // namespace WasmVM

#endif