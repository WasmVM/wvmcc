#ifndef WVMCC_SourceStream_DEF
#define WVMCC_SourceStream_DEF

#include <iostream>
#include <fstream>
#include <sstream>
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

template<class T> requires std::derived_from<T, std::basic_istream<typename T::char_type>>
struct SourceStream : public T {

    template<class... Args>
    SourceStream(const std::filesystem::path path, Args... args) : T(std::forward<Args...>(args)...), pos(path){}

    using int_type = T::int_type;
    using char_type = T::char_type;

    virtual int_type get();
    virtual std::basic_istream<char_type>& putback(char_type ch);
    SourcePos pos;
};

struct SourceFileStream : public SourceStream<std::ifstream> {
    SourceFileStream(const std::filesystem::path path);
    using SourceStream<std::ifstream>::get;
};

struct SourceTextStream : public SourceStream<std::istringstream> {
    SourceTextStream(const std::string source, const std::filesystem::path path = "");
    using SourceStream<std::istringstream>::get;
};

}

std::ostream& operator<<(std::ostream&, WasmVM::SourcePos&);

#endif