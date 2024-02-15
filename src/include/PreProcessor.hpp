#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <filesystem>
#include <stack>
#include <string>
#include <vector>
#include <list>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <iterator>
#include <optional>
#include <Token.hpp>
#include <SourceStream.hpp>

namespace WasmVM {

struct PreProcessor {

    struct PPToken : public std::optional<Token> {

        PPToken(std::nullopt_t n = std::nullopt) : std::optional<Token>(){}
        PPToken(Token& token) : std::optional<Token>(token){}

        template<typename T> requires TokenType::is_valid<T>::value
        inline bool hold() {
            return has_value() && std::holds_alternative<T>(value());
        }

        std::unordered_set<std::string> expanded;
        bool skipped = false;
    };

    PreProcessor(std::filesystem::path path);
    PreProcessor(std::filesystem::path path, std::string text);
    PPToken get();

#ifndef CCTEST
private:
#endif


    struct Line {
        LineStream() = default;

        template<std::input_iterator InputIt>
        LineStream(InputIt begin, InputIt end){
            cur = buffer.insert(buffer.begin(), begin, end);
        }

        PPToken get();
    };

    struct Scanner : public PPStream {
        Scanner(std::filesystem::path path);
        Scanner(std::filesystem::path path, std::string text);
        PPToken get();
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
        std::optional<std::vector<std::string>> params;
        std::vector<PPToken> replacement;
        inline operator std::string(){
            return name;
        }

        bool operator==(const Macro& op) const;
    };

    bool is_text = false;
    std::stack<TokenStream> streams;
    std::unordered_map<std::string, Macro> macros;

    static void skip_whitespace(PPToken& token, PPStream& stream);
    static bool replace_macro(PPToken& token, PPStream& stream, std::unordered_map<std::string, Macro> macro_map);
    static void perform_replace(LineStream& line, std::unordered_map<std::string, Macro>& macro_map);

    void if_directive(PPToken& token); // TODO:
    void elif_directive(PPToken& token); // TODO:
    void else_directive(PPToken& token); // TODO:
    void endif_directive(PPToken& token); // TODO:
    void define_directive(PPToken& token);
    void undef_directive(PPToken& token); // TODO:
    void pragma_directive(PPToken& token); // TODO:
    void include_directive(PPToken& token); // TODO:
    void line_directive(PPToken& token); // TODO:
    void error_directive(PPToken& token);
};

} // namespace WasmVM

#endif