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

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstdio>
#include <map>
#include <set>

#include <exception.hpp>
#include <WasmVM.hpp>
#include <structures/WasmModule.hpp>
#include <color.hpp>
#include <CommandParser.hpp>
#include <Archive.hpp>
#include <Linker.hpp>
#include <Compiler.hpp>

using namespace WasmVM;

int main(int argc, const char* argv[]){

    // Parse argv
    CommandParser args(argc, argv, {
        CommandParser::Optional("--version", "Show version", 0/*, "-v"*/),
        CommandParser::Optional("--output", "Output file name", 1, "-o"),
        CommandParser::Optional("--pp", "Preprocessor only", "-E"),
        CommandParser::Optional("--comp", "Compile only, do not assemble", "-S"),
        CommandParser::Optional("--as", "Compile and/or assemble, do not link", "-c"),
        CommandParser::Repeated("--includes", "Add directory to header files search list", "-I"),
        CommandParser::Repeated("--libraries", "Add directory to library files search list", "-L"),
        CommandParser::Repeated("--library", "Add library", "-l"),
        CommandParser::Fixed("input_files", "C source files (.c), Wasm binary file (.wasm) or WasmVM static library (.a)", (unsigned int)-1)
    },
        "wasmvm-cc : C compiler for WasmVM"
    );
    // Warnings
    Exception::Warning::regist([](std::string message){
        std::cerr << COLOR_Warning ": " << message << std::endl;
    });

    try{
        // Split source files & binary files
        if(!args["input_files"]){
            throw Exception::Exception("no input file");
        }
        std::vector<std::filesystem::path> source_files;
        std::vector<std::filesystem::path> wat_files;
        std::vector<std::pair<std::filesystem::path, bool>> binary_files; // (path, is_archive)
        {
            std::vector<std::string> input_files = std::get<std::vector<std::string>>(args["input_files"].value());
            for(std::string input_file : input_files){
                std::filesystem::path input_path(input_file);
                // Check existance
                if(!std::filesystem::exists(input_path)){
                    throw Exception::Exception(std::string("file ") + input_file + " not exist");
                }
                // Check extension
                if(input_path.extension() == ".c"){
                    source_files.emplace_back(std::filesystem::canonical(input_path));
                    continue;
                }else if(input_path.extension() == ".wat"){
                    wat_files.emplace_back(std::filesystem::canonical(input_path));
                    continue;
                }
                // Check magic
                std::ifstream fin(input_path, std::ios::binary | std::ios::in);
                uint32_t magic;
                fin.read((char*)&magic, 4);
                fin.close();
                if(magic == 0x6d736100 || magic == 0x52414d56){
                    binary_files.emplace_back(std::filesystem::canonical(input_path), magic == 0x52414d56);
                    continue;
                }
                // Unknown
                throw Exception::Exception(std::string("unknown file format of ") + input_file);
            }
        }

        // Check multiple files with output specified
        if(args["output"] && (args["pp"] || args["comp"] || args["as"])){
            if((source_files.size() + wat_files.size()) != 1){
                throw Exception::Exception("multiple files are prohibited while '--pp (-E)', '--comp (-S)' or '--as (-c)' are specified with '--output (-o)'");
            }
        }
        if(binary_files.size() > 0){
            Exception::Warning("binary files will be ignored while '--pp (-E)', '--comp (-S)' or '--as (-c)' are specified");
        }
        if((args["pp"] || args["comp"]) && (wat_files.size() > 0)){
            Exception::Warning("text assembly file will be ignored while '--pp (-E)' or '--comp (-S)' are specified");
        }

        std::map<std::filesystem::path, WasmModule> modules;

        // Compile
        std::vector<std::filesystem::path> include_paths;
        if(args["includes"]){
            std::vector<std::string> includes = std::get<std::vector<std::string>>(args["includes"].value());
            include_paths.assign(includes.begin(), includes.end());
        }
        Compiler compiler(include_paths);
        for(std::filesystem::path source_file : source_files){
            modules[source_file] = compiler.compile(source_file);
        }

        // Assemble
        for(std::filesystem::path wat_path : wat_files){
            std::ifstream fin(wat_path);
            WasmModule module = module_parse(fin);
            fin.close();
            if(args["as"]){
                if(args["output"]){
                    std::ofstream fout(std::get<std::string>(args["output"].value()), std::ios::binary | std::ios::out);
                    module_encode(module, fout);
                    fout.close();
                    return 0;
                }else{
                    std::ofstream fout(wat_path.replace_extension(".wasm"), std::ios::binary | std::ios::out);
                    module_encode(module, fout);
                    fout.close();
                }
            }else{
                modules[wat_path] = module;
            }
        }

        // Link
        Linker::Config linker_config;
        if(args["libraries"]){
            std::vector<std::string> libraries = std::get<std::vector<std::string>>(args["libraries"].value());
            for(std::string library_path : libraries){
                linker_config.library_paths.emplace_back(library_path);
            }
        }
        linker_config.library_paths.emplace_back(DEFAULT_MODULE_PATH);
        if(args["library"]){
            std::vector<std::string> libraries = std::get<std::vector<std::string>>(args["library"].value());
            for(std::string library : libraries){
                linker_config.libraries.emplace_back(library);
            }
        }
        Linker linker(linker_config);

    }catch(Exception::Exception &e){
        std::cerr << args.program.filename().string() << ": " COLOR_Error ": " << e.what() << std::endl;
        return -1;
    }
    return 0;
}