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

SourceBuf::SourceBuf(const std::filesystem::path filename) : fin(filename)
{
}

SourceBuf::~SourceBuf(){
    fin.close();
}
SourceBuf* SourceBuf::setbuf(char_type* s, std::streamsize n){
    std::string_view view(s, n);
    buf.assign(view.begin(), view.end());
    return this;
}
SourceBuf::pos_type SourceBuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode){
    buf.clear();
    fin.seekg(off, dir);
    return fin.tellg();
}
SourceBuf::pos_type SourceBuf::seekpos(pos_type pos, std::ios_base::openmode){
    buf.clear();
    fin.seekg(pos);
    return fin.tellg();
}
int SourceBuf::sync(){
    fin.seekg(-(off_type)buf.size(), std::ios::cur);
    buf.clear();
    return 0;
}
SourceBuf::int_type SourceBuf::pbackfail(int_type c){
    if(c == traits_type::eof()){
        fin.seekg(-(off_type)(buf.size() + 1), std::ios::cur);
        buf.clear();
        c = fin.get();
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
        ch = fin.get();
    }else{
        ch = buf.front();
        buf.pop_front();
    }
    if(ch == '?'){
        // Check trigraph
        int_type cur;
        if(buf.empty()){
            cur = fin.get();
        }else{
            cur = buf.front();
            buf.pop_front();
        }
        if(cur == '?'){
            if(buf.empty()){
                cur = fin.get();
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
            next = fin.get();
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
        return fin.peek();
    }else{
        return buf.front();
    }
}
std::streamsize SourceBuf::showmanyc(){
    auto pos = fin.tellg();
    fin.seekg(0, std::ios::end);
    std::streamsize result = (fin.tellg() - pos) + buf.size();
    fin.seekg(pos);
    return result;
}

SourceFile::SourceFile(const std::filesystem::path filename) :
    path(filename), buffer(filename)
{
    set_rdbuf(&buffer);
}
SourcePos& SourceFile::position(){
    return buffer.pos;
}