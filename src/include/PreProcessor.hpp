#ifndef WVMCC_PreProcessor_DEF
#define WVMCC_PreProcessor_DEF

#include <istream>
#include <sstream>
#include <filesystem>

namespace WasmVM {

template <typename CharT, typename Traits = std::char_traits<CharT>>
struct PreProcessor : public std::basic_istream<CharT, Traits> {

    PreProcessor(std::filesystem::path filepath, std::basic_istream<CharT, Traits>& stream)
        : std::basic_istream<CharT, Traits>(&buf), filepath(filepath), stream(stream){}

protected:
    std::filesystem::path filepath;
    std::basic_istream<CharT, Traits>& stream;
    std::basic_stringbuf<CharT, Traits> buf;

};

} // namespace WasmVM

#endif