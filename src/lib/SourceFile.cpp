// Copyright (C) 2023 Luis Hsu
// 
// wvmcc is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// wvmcc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with wvmcc. If not, see <http://www.gnu.org/licenses/>.

#include <SourceFile.hpp>

#include <string_view>

using namespace WasmVM;

std::ostream& operator<<(std::ostream& os, SourcePos& pos){
    return os << pos.line() << ":" << pos.col();
}

SourceBuf::SourceBuf(std::istream& stream) : stream(stream){}
SourceBuf* SourceBuf::setbuf(char_type* s, std::streamsize n){
    std::string_view view(s, n);
    buf.assign(view.begin(), view.end());
    return this;
}
SourceBuf::pos_type SourceBuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode){
    buf.clear();
    stream.seekg(off, dir);
    return stream.tellg();
}
SourceBuf::pos_type SourceBuf::seekpos(pos_type pos, std::ios_base::openmode){
    buf.clear();
    stream.seekg(pos);
    return stream.tellg();
}
int SourceBuf::sync(){
    stream.seekg(-(off_type)buf.size(), std::ios::cur);
    buf.clear();
    return 0;
}
SourceBuf::int_type SourceBuf::pbackfail(int_type c){
    if(c == traits_type::eof()){
        stream.seekg(-(off_type)(buf.size() + 1), std::ios::cur);
        buf.clear();
        c = stream.get();
    }
    if(c == '\n' && pos.line() != 1){
        pos.line() -= 1;
    }else if(pos.col() != 1){
        pos.col() -= 1;
    }
    buf.push_front(c);
    return 0;
}
SourceBuf::int_type SourceBuf::uflow(){
    int_type ch;
    if(buf.empty()){
        ch = stream.get();
    }else{
        ch = buf.front();
        buf.pop_front();
    }
    if(ch == '?'){
        // Check trigraph
        int_type cur;
        if(buf.empty()){
            cur = stream.get();
        }else{
            cur = buf.front();
            buf.pop_front();
        }
        if(cur == '?'){
            if(buf.empty()){
                cur = stream.get();
            }else{
                cur = buf.front();
                buf.pop_front();
            }
            switch(cur){
                case '=':
                    pos.col() += 3;
                    return '#';
                break;
                case ')':
                    pos.col() += 3;
                    return ']';
                break;
                case '!':
                    pos.col() += 3;
                    return '|';
                break;
                case '(':
                    pos.col() += 3;
                    return '[';
                break;
                case '\'':
                    pos.col() += 3;
                    return '^';
                break;
                case '>':
                    pos.col() += 3;
                    return '}';
                break;
                case '<':
                    pos.col() += 3;
                    return '{';
                break;
                case '/':
                    pos.col() += 3;
                    return '\\';
                break;
                case '-':
                    pos.col() += 3;
                    return '~';
                break;
                default:
                    buf.push_front('?');
                    buf.push_front(cur);
            }
        }else{
            buf.push_front(cur);
        }
    }else if(ch == '\\'){
        int_type next;
        if(buf.empty()){
            next = stream.get();
        }else{
            next = buf.front();
            buf.pop_front();
        }
        if(next == '\n'){
            pos.line() += 1;
            pos.col() = 0;
            return uflow();
        }else{
            buf.push_front(next);
        }
    }else if(ch == '\n'){
        pos.line() += 1;
        pos.col() = 0;
        return ch;
    }
    pos.col() += 1;
    return ch;
}
SourceBuf::int_type SourceBuf::underflow(){
    if(buf.empty()){
        return stream.peek();
    }else{
        return buf.front();
    }
}
std::streamsize SourceBuf::showmanyc(){
    auto pos = stream.tellg();
    stream.seekg(0, std::ios::end);
    std::streamsize result = (stream.tellg() - pos) + buf.size();
    stream.seekg(pos);
    return result;
}

SourceFileBuf::SourceFileBuf(const std::filesystem::path filename) : SourceBuf(fin), fin(filename)
{}
SourceFileBuf::~SourceFileBuf(){
    fin.close();
}

SourceTextBuf::SourceTextBuf(std::string source) : SourceBuf(tin), tin(source)
{}

SourceFile::SourceFile(const std::filesystem::path filename) :
    path(filename), buffer(new SourceFileBuf(filename))
{
    set_rdbuf(buffer.get());
}
SourceFile::SourceFile(const std::filesystem::path filename, std::string source) : 
    path(filename), buffer(new SourceTextBuf(source))
{
    set_rdbuf(buffer.get());
}
SourcePos& SourceFile::position(){
    return buffer->pos;
}