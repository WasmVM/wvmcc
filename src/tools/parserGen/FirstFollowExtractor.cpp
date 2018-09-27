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

#include <iostream>

#include <parserGen/FirstFollowExtractor.hpp>

static void extractFirstSet(RuleMap* ruleMap, FirstFollow* firstSet){
    bool result = false;
    do{
        result = false;
        for(RuleMap::iterator targetIt = ruleMap->begin(); targetIt != ruleMap->end(); ++targetIt){
            for(std::vector<Rule>::iterator ruleIt = targetIt->second.begin(); ruleIt != targetIt->second.end(); ++ruleIt){
                if(ruleIt->elements.size() == 0){
                    result |= firstSet->addElement(ruleIt->target, "");
                }else if(ruleMap->find(ruleIt->elements.at(0)) != ruleMap->end()){
                    if(firstSet->find(ruleIt->elements.at(0)) != firstSet->end()){
                        std::list<std::string>& targetList = firstSet->find(ruleIt->elements.at(0))->second;
                        for(std::list<std::string>::iterator it = targetList.begin(); it != targetList.end(); ++it){
                            result |= firstSet->addElement(ruleIt->target, *it);
                        }
                    }
                }else{
                    result |= firstSet->addElement(ruleIt->target, ruleIt->elements.at(0));
                }
            }
        }
    }while(result);
}

static void extractFollowSet(RuleMap* ruleMap, const std::string start, FirstFollow* firstSet, FirstFollow* followSet){
    bool result = false;
    followSet->addElement(start, endFlag);
    do{
        result = false;
        for(RuleMap::iterator targetIt = ruleMap->begin(); targetIt != ruleMap->end(); ++targetIt){
            for(std::vector<Rule>::iterator ruleIt = targetIt->second.begin(); ruleIt != targetIt->second.end(); ++ruleIt){
                for(std::vector<std::string>::iterator elemIt = ruleIt->elements.begin(); elemIt != ruleIt->elements.end(); ++elemIt){
                    if(ruleMap->find(*elemIt) != ruleMap->end()){
                        if(elemIt == (ruleIt->elements.end() - 1)){
                            if(followSet->find(ruleIt->target) != followSet->end()){
                                std::list<std::string>& targetList = followSet->find(ruleIt->target)->second;
                                for(std::list<std::string>::iterator it = targetList.begin(); it != targetList.end(); ++it){
                                    result |= followSet->addElement(*elemIt, *it);
                                }
                            }
                        }else{
                            FirstFollowMap::iterator nextIt = firstSet->find(elemIt[1]);
                            if(nextIt == firstSet->end()){
                                result |= followSet->addElement(*elemIt, elemIt[1]);
                            }else{
                                for(std::list<std::string>::iterator it = nextIt->second.begin(); it != nextIt->second.end(); ++it){
                                    result |= followSet->addElement(*elemIt, *it);
                                }
                            }
                        }
                    }
                }
            }
        }
    }while(result);
}

static int run_FirstFollowExtractor(Pass* pass){
    RuleMap* ruleMap = (RuleMap*) pass->input[0]->data;
    FirstFollow* firstSet = (FirstFollow*) pass->output[0];
    FirstFollow* followSet = (FirstFollow*) pass->output[1];
    char* startStr = nullptr;
    if(mapGet(pass->contextMap, (void*)"start", (void**)&startStr)){
        return -1;
    }
    // FirstSet
    extractFirstSet(ruleMap, firstSet);
    // FollowSet
    extractFollowSet(ruleMap, std::string(startStr), firstSet, followSet);
    return 0;
}

static void free_FirstFollowExtractor(Pass** passPtr){
    Pass* pass = *passPtr;
    delete pass->input;
    delete [] pass->output;
    free(pass);
    *passPtr = NULL;
}

FirstFollowExtractor::FirstFollowExtractor(RuleBuffer& buffer):
    pass((Pass*)malloc(sizeof(Pass))), firstSet(new FirstFollow), followSet(new FirstFollow)
{
    pass->input = new Buffer* {buffer.getBuffer()};
    pass->input_count = 1;
    pass->output = new Buffer*[2]{(Buffer*)firstSet, (Buffer*)followSet};
    pass->output_count = 2;
    pass->contextMap = NULL;
    pass->run = run_FirstFollowExtractor;
    pass->free = free_FirstFollowExtractor;
}
Pass* & FirstFollowExtractor::getPass(){
    return pass;
}
FirstFollow* & FirstFollowExtractor::getFirstSet(){
    return firstSet;
}
FirstFollow* & FirstFollowExtractor::getFollowSet(){
    return followSet;
}