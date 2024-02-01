#ifndef WVMCC_SourceFile_DEF
#define WVMCC_SourceFile_DEF

#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <stack>
#include <utility>
#include <memory>

namespace WasmVM {

struct SourcePos : public std::pair<size_t, size_t> {
    SourcePos(size_t l = 1, size_t c = 0) : std::pair<size_t, size_t>(l, c){}
    inline size_t& line() {return first;}
    inline size_t& col() {return second;}
};

class SourceBuf : public std::filebuf {
    std::istream& stream;
protected:
    std::deque<int_type> buf;
    SourcePos pos;
    
    SourceBuf(std::istream& stream);
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

class SourceFileBuf : public SourceBuf {
    std::ifstream fin;

    SourceFileBuf(const std::filesystem::path filename);
    ~SourceFileBuf();

    friend class SourceFile;
};

class SourceTextBuf : public SourceBuf {
    std::istringstream tin;

    SourceTextBuf(std::string source);

    friend class SourceFile;
};

struct SourceFile : public std::ifstream {
    SourceFile(const std::filesystem::path filename);
    SourceFile(const std::filesystem::path filename, std::string source);
    SourcePos& position();
    std::filesystem::path path;
private:
    std::unique_ptr<SourceBuf> buffer;
};

}

std::ostream& operator<<(std::ostream&, WasmVM::SourcePos&);

#endif