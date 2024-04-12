// Copyright 2023 Luis Hsu. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "CommandParser.hpp"

#include <queue>
#include <iostream>
#include <cstdlib>

#include <Util.hpp>
#include "color.hpp"

static void help(std::filesystem::path program, std::string desc,
    std::vector<CommandParser::Opt> options
){
    std::cerr << desc << std::endl;
    // Usage
    std::cerr << std::endl << "Usage: " << program.filename().string() << " [-h | --help]";
    for(CommandParser::Opt option : options){
        std::visit(overloaded {
            [&](CommandParser::Fixed& opt){
                std::cerr << " " << opt.name;
                switch(opt.number){
                    case (unsigned)-1:
                        std::cerr << "...";
                    break;
                    case 0:
                    case 1:
                    break;
                    default:
                        std::cerr << "{" << opt.number << "}";
                    break;
                }
            },
            [&](CommandParser::Optional& opt){
                std::cerr << " [";
                if(!opt.alias.empty()){
                    std::cerr << opt.alias << " | ";
                }
                std::cerr << opt.name << " ]";
                switch(opt.number){
                    case (unsigned)-1:
                        std::cerr << "...";
                    break;
                    case 0:
                    break;
                    case 1:
                        std::cerr << "?";
                    break;
                    default:
                        std::cerr << "{" << opt.number << "}";
                    break;
                }
            },
            [&](CommandParser::Repeated& opt){
                std::cerr << " [";
                if(!opt.alias.empty()){
                    std::cerr << opt.alias << "? | ";
                }
                std::cerr << opt.name << " ?]...";
            }
        }, option);
    }
    
    // Options
    std::cerr << std::endl << std::endl << "Options:" << std::endl;
    std::cerr << "  --help, -h\t\tShow help message" << std::endl;
    for(auto& option : options){
        std::visit(overloaded {
            [&](CommandParser::Fixed& opt){
                std::cerr << "  " << opt.name << "\t\t" << opt.desc << std::endl;
            },
            [&](CommandParser::Optional& opt){
                std::cerr << "  " << opt.name;
                if(!opt.alias.empty()){
                    std::cerr << ", " << opt.alias;
                }
                std::cerr << "\t\t" << opt.desc<< std::endl;
            },
            [&](CommandParser::Repeated& opt){
                std::cerr << "  " << opt.name;
                if(!opt.alias.empty()){
                    std::cerr << ", " << opt.alias;
                }
                std::cerr << "\t\t" << opt.desc<< std::endl;
            }
        }, option);
    }
    std::cerr << std::endl;
    // Symbol notes
    std::cerr << "Symbols:" << std::endl;
    std::cerr << "  []\t\tOptional, the argument is not required" << std::endl;
    std::cerr << "  |\t\tAlias, the left side and right side are both accepted" << std::endl;
    std::cerr << "  ?\t\tOperand, consume a string sequence or one more argument as operand of this argument" << std::endl;
    std::cerr << "  ...\t\tRepeat, the argument can be specified more than once" << std::endl;
    std::cerr << std::endl;
}

CommandParser::CommandParser(int argc, const char* argv[],
    std::initializer_list<CommandParser::Opt> options, std::string desc
) : program(argv[0]){

    std::queue<Fixed> fixed;
    std::unordered_map<std::string, std::variant<Optional, Repeated>> optional;
    std::unordered_map<std::string, std::string> aliases;
    for(auto option : options){
        std::visit(overloaded {
            [&](Fixed& opt){
                Fixed& arg = fixed.emplace(opt);
                if(arg.number == 0){
                    arg.number = 1;
                }
            },
            [&](Optional& opt){
                optional[opt.name] = opt;
                if(!opt.alias.empty()){
                    aliases[opt.alias] = opt.name;
                }
            },
            [&](Repeated& opt){
                optional[opt.name] = opt;
                if(!opt.alias.empty()){
                    aliases[opt.alias] = opt.name;
                }
            }
        }, option);
    }

    int argi = 1;
    while(argi < argc){
        std::string arg(argv[argi]);
        if(arg == "--help" || arg == "-h"){
            help(program, desc, options);
            std::exit(0);
        }else if(optional.contains(arg) || aliases.contains(arg)){
            if(aliases.contains(arg)){
                arg = aliases[arg];
            }
            Arg& value = args[arg];
            argi += 1;
            std::visit(overloaded {
                [&](Optional& opt){
                    switch(opt.number){
                        case 0:
                            value.emplace<std::monostate>();
                        break;
                        case 1:
                            value.emplace<std::string>(argv[argi]);
                            argi += 1;
                        break;
                        default:{
                            std::vector<std::string>& values = value.emplace<std::vector<std::string>>();
                            for(unsigned remain = opt.number; remain > 0 && argi < argc; --remain, ++argi){
                                std::string argstr(argv[argi]);
                                if(optional.contains(argstr) || aliases.contains(argstr)){
                                    break;
                                }else if(argstr == "--help" || argstr == "-h"){
                                    help(program, desc, options);
                                    std::exit(0);
                                }
                                values.emplace_back(argstr);
                            }
                        }break;
                    }
                },
                [&](Repeated&){
                    if(std::holds_alternative<std::monostate>(value)){
                        value.emplace<std::vector<std::string>>();
                    }
                    std::vector<std::string>& values = std::get<std::vector<std::string>>(value);
                    values.emplace_back(argv[argi]);
                    argi += 1;
                }
            }, optional[arg]);
            continue;
        }
        bool consumed = false;
        for(auto alias_pair : aliases){
            if(arg.starts_with(alias_pair.first) && std::holds_alternative<Repeated>(optional[alias_pair.second])){
                arg = arg.substr(alias_pair.first.size());
                Arg& value = args[alias_pair.second];
                argi += 1;
                if(std::holds_alternative<std::monostate>(value)){
                    value.emplace<std::vector<std::string>>();
                }
                std::vector<std::string>& values = std::get<std::vector<std::string>>(value);
                values.emplace_back(arg);
                consumed = true;
                break;
            }
        }
        if(consumed){
            continue;
        }
        if(!fixed.empty()){
            Fixed& option = fixed.front();
            Arg& value = args[option.name];
            bool do_pop = true;
            switch(option.number){
                case 1:
                    value.emplace<std::string>(arg);
                    argi += 1;
                break;
                default:{
                    std::vector<std::string>& values = std::holds_alternative<std::vector<std::string>>(value) ? std::get<std::vector<std::string>>(value) : value.emplace<std::vector<std::string>>();
                    for(unsigned remain = option.number; remain > 0 && argi < argc; --remain, ++argi){
                        std::string argstr(argv[argi]);
                        if(optional.contains(argstr) || aliases.contains(argstr)){
                            do_pop = false;
                            break;
                        }else if(argstr == "--help" || argstr == "-h"){
                            help(program, desc, options);
                            std::exit(0);
                        }
                        values.emplace_back(argstr);
                    }
                }break;
            }
            if(do_pop){
                fixed.pop();
            }
        }else{
            // Too many arguments
            std::cerr << COLOR_Error ": too many arguments" << std::endl;
            help(program, desc, options);
            std::exit(-1);
        }
    }
}

std::optional<CommandParser::Arg> CommandParser::operator[](const std::string key){
    if(args.contains(key)){
        return args[key];
    }else if(args.contains(std::string("--") + key)){
        return args[std::string("--") + key];
    }
    return std::nullopt;
}