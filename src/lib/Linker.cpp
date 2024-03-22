// Copyright 2023 Luis Hsu. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

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