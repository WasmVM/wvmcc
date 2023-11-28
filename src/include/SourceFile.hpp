#ifndef WVMCC_SourceFile_DEF
#define WVMCC_SourceFile_DEF

#include <fstream>
#include <deque>

namespace WasmVM {

class SourceBuf : public std::filebuf {
    std::deque<int_type> buf;
    std::ifstream fin;

    SourceBuf(const std::filesystem::path filename);
    ~SourceBuf();
    SourceBuf* setbuf(char_type* s, std::streamsize n);
    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in);
    pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in);
    int sync();
    int_type pbackfail(int_type c = SourceBuf::traits_type::eof());
    int_type uflow();
    int_type underflow();
    std::streamsize showmanyc();

    friend class SourceFile;
};

struct SourceFile : public std::ifstream {
    SourceFile(const std::filesystem::path filename);
private:
    SourceBuf buffer;
};

}

#endif