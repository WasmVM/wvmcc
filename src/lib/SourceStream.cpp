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

#include <SourceStream.hpp>

#include <string_view>

using namespace WasmVM;

std::ostream& operator<<(std::ostream& os, SourcePos& pos){
    return os << pos.line() << ":" << pos.col();
}

template<class T> requires std::derived_from<T, std::basic_istream<typename T::char_type>>
std::basic_istream<typename T::char_type>& SourceStream<T>::putback(SourceStream<T>::char_type ch){
    if(ch == '\n' && pos.line() != 1){
        pos.line() -= 1;
    }else if(pos.col() != 1){
        pos.col() -= 1;
    }
    return T::putback(ch);
}

template<class T> requires std::derived_from<T, std::basic_istream<typename T::char_type>>
SourceStream<T>::int_type SourceStream<T>::get(){
    int_type ch = T::get();
    if(ch == '?'){
        // Check trigraph
        int_type cur = T::get();
        if(cur == '?'){
            cur = T::get();
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
                    T::putback(cur);
            }
        }
        T::putback('?');
    }else if(ch == '\\'){
        int_type cur = T::get();
        if(cur == '\n'){
            pos.line() += 1;
            pos.col() = 0;
            return get();
        }
        T::putback(cur);
    }else if(ch == '\n'){
        pos.line() += 1;
        pos.col() = 0;
        return ch;
    }
    pos.col() += 1;
    return ch;
}

SourceFileStream::SourceFileStream(const std::filesystem::path path) : SourceStream<std::ifstream>(path, path){}

SourceTextStream::SourceTextStream(const std::string source, const std::filesystem::path path) : SourceStream<std::istringstream>(path, source){}