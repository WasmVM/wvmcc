// Copyright (C) 2024 Luis Hsu
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
// along with wvmcc. If not, see <https://www.gnu.org/licenses/>.

#include "Linker.hpp"

#include <filesystem>
#include <fstream>
#include <vector>
#include <Util.hpp>
#include <exception.hpp>

using namespace WasmVM;

Linker::Linker(Config config): config(config){
    // Find libraries
    for(std::string library : config.libraries){
        std::optional<std::filesystem::path> library_file;
        for(std::filesystem::path library_path : config.library_paths){
            library_path = library_path / library;
            if(std::filesystem::exists(library_path)
                || std::filesystem::exists(library_path.replace_extension(".a"))
                || std::filesystem::exists(library_path.replace_extension(".wasm"))
                || std::filesystem::exists(library_path.replace_filename(std::string("lib") + library_path.filename().string()))
                || std::filesystem::exists(library_path.replace_extension(".a"))
            ){
                library_file.emplace(library_path);
                break;
            }
        }
        if(!library_file){
            throw Exception::Exception(std::string("library '") + library + "' not found");
        }
        std::optional<Linker::LibraryType> library_type = check_library_type(library_file.value());
        if(!library_type){
            throw Exception::Exception(std::string("unknown format  of library '") + library + "'");
        } 
        libraries.emplace_back(library_file.value(), library_type.value());
    }
}

std::optional<Linker::LibraryType> Linker::check_library_type(std::filesystem::path path){
    std::ifstream fin(path, std::ios::binary);
    uint32_t magic;
    fin.read((char*)&magic, sizeof(uint32_t));
    fin.close();
    if(magic == 0x52414d56){ // Archive
        return Linker::LibraryType::Static;
    }else if(magic == 0x6d736100){ // Wasm
        return Linker::LibraryType::Shared;
    }else{
        return std::nullopt;
    }
}