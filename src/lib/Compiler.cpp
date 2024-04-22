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

#include <PreProcessor.hpp>
#include <Lexer.hpp>
#include <Parser.hpp>
#include <AST/Dump.hpp>

using namespace WasmVM;

Compiler::Compiler(std::vector<std::filesystem::path> include_paths) :
    include_paths(include_paths)
{
}

WasmModule Compiler::compile(std::filesystem::path source_path){
    PreProcessor pp(source_path, include_paths);
    Lexer lexer(pp);
    Parser parser(lexer);

    std::cout << parser.parse<AST::PrimaryExpr>().value() << std::endl;
    
    return WasmModule(); // FIXME:
}