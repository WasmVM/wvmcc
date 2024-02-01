#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <filesystem>
#include <stack>
#include <string>
#include <vector>
#include <list>
#include <type_traits>
#include <functional>
#include <unordered_set>
#include <utility>
#include <iterator>
#include <optional>
#include <Token.hpp>
#include <SourceFile.hpp>

namespace WasmVM {

struct PreProcessor {

    struct PPToken : public std::optional<Token> {

        PPToken(std::nullopt_t n = std::nullopt) : std::optional<Token>(){}
        PPToken(Token&& token) : std::optional<Token>(token){}

        template<typename T> requires TokenType::is_valid<T>::value
        inline bool hold() {
            return has_value() && std::holds_alternative<T>(value());
        }
    };

    PreProcessor(std::filesystem::path path);
    PreProcessor(std::filesystem::path path, std::string text);
    PPToken get();

#ifndef CCTEST
private:
#endif

    struct PPStream {
        virtual PPToken get() = 0;
    };

    struct LineStream : public PPStream {
        PPToken get();
        std::list<Token> buffer;
    };

    struct TokenStream : public PPStream {
        TokenStream(std::filesystem::path path);
        TokenStream(std::filesystem::path path, std::string text);
        PPToken get();
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
        std::optional<std::vector<std::string>> params;
        std::vector<Token> replacement;
        inline operator std::string(){
            return name;
        }

        bool operator==(const Macro& op) const;

        struct Hash {
            std::size_t operator()(const Macro& macro) const{
                return std::hash<std::string>{}(macro.name);
            }
        };

        struct NameEqual {
            constexpr bool operator()(const Macro& lhs, const Macro& rhs) const{
                return lhs.name == rhs.name;
            }
        };
    };

    bool is_text = false;
    std::stack<TokenStream> streams;
    std::unordered_set<Macro, Macro::Hash, Macro::NameEqual> macros;

    void skip_whitespace(PPToken& token);
    PPToken& replace_macro(PPToken& token, TokenStream& stream);
    PPToken& replace_macro(PPToken& token, LineStream& stream, std::vector<std::list<Token>> args);

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