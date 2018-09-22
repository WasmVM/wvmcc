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

#include <parserGen/Rule.hpp>

static void free_RuleBuffer_data(void* data){
    RuleMap* ruleMap = (RuleMap*)data;
    delete ruleMap;
}
static void free_RuleBuffer(Buffer** bufferPtr){
    RuleBuffer* ruleBuffer = (RuleBuffer*)*bufferPtr;
    delete ruleBuffer;
    *bufferPtr = nullptr;
}
Rule::Rule(std::string target):
    target(target){
}
Rule::Element::Element(std::string name, bool isOptional):
    name(name), isOptional(isOptional){
}
void Rule::print(){
    std::cout << target << " => ";
    for(std::vector<Element>::iterator it = elements.begin(); it != elements.end(); ++it){
        std::cout << it->name;
        if(it->isOptional){
            std::cout << "?";
        }
        std::cout << " ";
    }
    std::cout << std::endl;
}
void Rule::addElement(std::string name, bool isOptional){
    elements.push_back(Element(name, isOptional));
}
RuleBuffer::RuleBuffer(){
    buffer.data = new RuleMap;
    buffer.length = 0;
    buffer.free = free_RuleBuffer;
}
RuleBuffer::~RuleBuffer(){
    free_RuleBuffer_data(buffer.data);
}
Buffer* RuleBuffer::getBuffer(){
    return &buffer;
}
void RuleBuffer::addRule(const Rule& rule){
    RuleMap& ruleMap = *(RuleMap*)buffer.data;
    ruleMap[rule.target].push_back(rule);
    buffer.length = ruleMap.size();
}
Rule& RuleBuffer::getRule(const std::string target, const size_t index){
    RuleMap& ruleMap = *(RuleMap*)buffer.data;
    return ruleMap.at(target).at(index);
}
RuleMap::iterator RuleBuffer::getTargetRules(const std::string target){
    RuleMap& ruleMap = *(RuleMap*)buffer.data;
    return ruleMap.find(target);
}
RuleMap::iterator RuleBuffer::begin(){
    RuleMap* ruleMap = (RuleMap*)buffer.data;
    return ruleMap->begin();
}
RuleMap::iterator RuleBuffer::end(){
    RuleMap* ruleMap = (RuleMap*)buffer.data;
    return ruleMap->end();
}
RuleMap::iterator RuleBuffer::find(const std::string target){
    RuleMap& ruleMap = *(RuleMap*)buffer.data;
    return ruleMap.find(target);
}