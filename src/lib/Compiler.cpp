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

#include <Compiler.hpp>

#include <iostream> // FIXME:
#include <SourceFile.hpp>

using namespace WasmVM;

Compiler::Compiler(std::vector<std::filesystem::path> include_paths) :
    include_paths(include_paths)
{
}

WasmModule Compiler::compile(std::filesystem::path source_path){
    SourceFile source(source_path);
    // TODO:
    for(int ch = source.get(); ch != SourceFile::traits_type::eof(); ch = source.get()){
        std::cout << "'" << (char)ch << "' " << source.position() << std::endl;
    }
    return WasmModule(); // FIXME:
}