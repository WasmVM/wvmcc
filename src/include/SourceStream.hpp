#ifndef WVMCC_SourceStream_DEF
#define WVMCC_SourceStream_DEF

#include <iostream>
#include <utility>
#include <concepts>
#include <filesystem>

namespace WasmVM {

struct SourcePos : public std::pair<size_t, size_t> {
    SourcePos(std::filesystem::path f = "", size_t l = 1, size_t c = 0) : std::pair<size_t, size_t>(l, c), path(f){}
    SourcePos(const SourcePos& s) : std::pair<size_t, size_t>(s.first, s.second), path(s.path){}
    std::filesystem::path path;
    inline size_t& line() {return first;}
    inline size_t& col() {return second;}
};

template<typename IntType, typename CharType> requires std::integral<IntType> && std::integral<CharType>
struct SourceStreamBase {
    virtual ~SourceStreamBase(){};
    virtual IntType get() = 0;
    virtual std::basic_istream<CharType>& putback(CharType ch) = 0;
    virtual std::basic_istream<CharType>& unget() = 0;
    virtual IntType peek() = 0;
    SourcePos pos;
};

template<class T> requires std::derived_from<T, std::basic_istream<typename T::char_type>>
struct SourceStream : public T, SourceStreamBase<typename T::int_type, typename T::char_type> {

    using int_type = T::int_type;
    using char_type = T::char_type;
    using base_type = SourceStreamBase<int_type, char_type>;

    template<class... Args>
    SourceStream(const std::filesystem::path path, Args... args) : T(std::forward<Args...>(args)...){
        base_type::pos.path = path;
    }

    virtual int_type get(){
        int_type ch = T::get();
        if(ch == '?'){
            // Check trigraph
            int_type cur = T::get();
            if(cur == '?'){
                cur = T::get();
                switch(cur){
                    case '=':
                        base_type::pos.col() += 3;
                        return '#';
                    break;
                    case ')':
                        base_type::pos.col() += 3;
                        return ']';
                    break;
                    case '!':
                        base_type::pos.col() += 3;
                        return '|';
                    break;
                    case '(':
                        base_type::pos.col() += 3;
                        return '[';
                    break;
                    case '\'':
                        base_type::pos.col() += 3;
                        return '^';
                    break;
                    case '>':
                        base_type::pos.col() += 3;
                        return '}';
                    break;
                    case '<':
                        base_type::pos.col() += 3;
                        return '{';
                    break;
                    case '/':
                        base_type::pos.col() += 3;
                        return '\\';
                    break;
                    case '-':
                        base_type::pos.col() += 3;
                        return '~';
                    break;
                    default:
                        T::unget();
                }
            }
            T::unget();
        }else if(ch == '\\'){
            int_type cur = T::get();
            if(cur == '\n'){
                base_type::pos.line() += 1;
                base_type::pos.col() = 0;
                return get();
            }
            T::unget();
        }else if(ch == '\n'){
            base_type::pos.line() += 1;
            base_type::pos.col() = 0;
            return ch;
        }
        if(ch != std::char_traits<char_type>::eof()){
            base_type::pos.col() += 1;
        }
        return ch;
    }
    virtual int_type peek(){
        return T::peek();
    }
    virtual std::basic_istream<char_type>& putback(char_type ch){
        if(ch == '\n' && base_type::pos.line() != 1){
            base_type::pos.line() -= 1;
        }else if(base_type::pos.col() != 1){
            base_type::pos.col() -= 1;
        }
        return T::putback(ch);
    }
    virtual std::basic_istream<char_type>& unget(){
        if(T::rdbuf()->sungetc() == '\n'&& base_type::pos.line() != 1){
            base_type::pos.line() -= 1;
        }else if(base_type::pos.col() != 1){
            base_type::pos.col() -= 1;
        }
        return *this;
    }
    
};

}

std::ostream& operator<<(std::ostream&, WasmVM::SourcePos&);

#endif