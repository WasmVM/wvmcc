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

#include <PreProcessor.hpp>

#include "Visitor.hpp"

using namespace WasmVM;

PreProcessor::PreProcessor(std::filesystem::path filepath){
    visitors.emplace(new Visitor(filepath, *this));
}

std::optional<PreProcessor::Token> PreProcessor::get(){
    while(!visitors.empty() && buffer.empty()){
        std::shared_ptr<Visitor> visitor = visitors.top();
        if(!visitors.top()->fetch()){
            visitors.pop();
        }
    }
    if(visitors.empty() && buffer.empty()){
        return std::nullopt;
    }
    Token token = buffer.front();
    buffer.pop_front();
    return token;
}