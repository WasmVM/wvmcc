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
Rule::Rule(const Rule &rule){
    target = rule.target;
    elements.assign(rule.elements.begin(), rule.elements.end());
}
void Rule::print(){
    std::cout << target << " => ";
    for(std::vector<std::string>::iterator it = elements.begin(); it != elements.end(); ++it){
        std::cout << *it;
        std::cout << " ";
    }
    std::cout << std::endl;
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