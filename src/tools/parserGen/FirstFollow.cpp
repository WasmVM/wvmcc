//    Copyright 2018 Luis Hsu
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <parserGen/FirstFollow.hpp>

static void free_FirstFollow_data(void* data){
    FirstFollowMap* firstfollowMap = (FirstFollowMap*)data;
    delete firstfollowMap;
}
static void free_FirstFollow(Buffer** bufferPtr){
    FirstFollow* firstfollow = (FirstFollow*)*bufferPtr;
    delete firstfollow;
    *bufferPtr = nullptr;
}
FirstFollow::FirstFollow(){
    buffer.data = new FirstFollowMap;
    buffer.length = 0;
    buffer.free = free_FirstFollow;
}
FirstFollow::~FirstFollow(){
    free_FirstFollow_data(buffer.data);
}
Buffer* FirstFollow::getBuffer(){
    return &buffer;
}
FirstFollowMap::iterator FirstFollow::begin(){
    FirstFollowMap* firstfollowMap = (FirstFollowMap*)buffer.data;
    return firstfollowMap->begin();
}
FirstFollowMap::iterator FirstFollow::end(){
    FirstFollowMap* firstfollowMap = (FirstFollowMap*)buffer.data;
    return firstfollowMap->end();
}
FirstFollowMap::iterator FirstFollow::find(const std::string target){
    FirstFollowMap* firstfollowMap = (FirstFollowMap*)buffer.data;
    return firstfollowMap->find(target);
}
bool FirstFollow::addElement(const std::string target, const std::string element){
    FirstFollowMap* firstfollowMap = (FirstFollowMap*)buffer.data;
    std::list<std::string>& elements = (*firstfollowMap)[target];
    for(std::list<std::string>::iterator it = elements.begin(); it != elements.end(); ++it){
        if(*it == element){
            return false;
        }
    }
    elements.push_back(element);
    return true;
}
void FirstFollow::print(){
    for(FirstFollowMap::iterator mapIt = begin(); mapIt != end(); ++mapIt){
        std::cout << mapIt->first << " :";
        for(std::list<std::string>::iterator elemIt = mapIt->second.begin(); elemIt != mapIt->second.end(); ++elemIt){
            std::cout << " " << ((*elemIt == "") ? "\\0" : *elemIt);
        }
        std::cout << std::endl;
    }
}