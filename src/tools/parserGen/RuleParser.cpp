#include <parserGen/RuleParser.hpp>

static std::string trim(const std::string str){
    size_t start = str.find_first_not_of(" \t\v\r"), last = str.find_last_not_of(" \t\v\r\n");
    if(start == std::string::npos || last == std::string::npos){
        return std::string("");
    }
    return str.substr(start, str.find_last_not_of(" \t\v\r\n") + 1 - start);
}

static int parseLine(std::string lineStr, RuleBuffer* ruleBuffer){
    // Create rule
    size_t splitPos = lineStr.find_first_of(':');
    if(splitPos == std::string::npos){
        return -1;
    }
    Rule rule(trim(lineStr.substr(0, splitPos)));
    lineStr = trim(lineStr.substr(splitPos + 1));
    // Fill elements
    while(lineStr.find_first_not_of(" \t\v\r\n") != std::string::npos){
        std::string elementStr(lineStr.substr(0, lineStr.find_first_of(" \t\v\r\n")));
        lineStr = trim(lineStr.substr(elementStr.size()));
        rule.elements.push_back(elementStr);
    }
    // Expand optional
    std::queue<Rule> ruleQueue;
    ruleQueue.push(rule);
    while(!ruleQueue.empty()){
        Rule nextRule = ruleQueue.front();
        ruleQueue.pop();
        bool hasOptional = false;
        size_t index = 0;
        for(std::vector<std::string>::iterator it = nextRule.elements.begin(); it != nextRule.elements.end(); ++it, ++index){
            if((it->back() == '?') && (it->size() != 1)){
                hasOptional = true;
                Rule forkedRule(nextRule);
                forkedRule.elements.erase(forkedRule.elements.begin() + index);
                ruleQueue.push(forkedRule);
                it->pop_back();
            }
        }
        if(hasOptional){
            ruleQueue.push(nextRule);
        }else{
            ruleBuffer->addRule(nextRule);
        }
    }
    return 0;
}

static int run_RuleParser(Pass* pass){
    // Check output count
    if(pass->output_count != 1){
        std::cerr << "[RuleParser] error: There must be exactly 1 output." << std::endl;
        return -1;
    }
    // Prepare output
    RuleBuffer* ruleBuffer = (RuleBuffer*)*pass->output;
    // Run
    for(size_t inputIndex = 0; inputIndex < pass->input_count; ++inputIndex){
        Buffer* input = pass->input[inputIndex];
        char* line = strtok((char*)input->data, "\n");
        while(line != NULL){
            int ret = parseLine(std::string(line), ruleBuffer);
            if(ret){
                return ret;
            }
            line = strtok(NULL, "\n");
        }
    }
    return 0;
}

static void free_RuleParser(Pass** pass){
    free(*pass);
    *pass = NULL;
}

RuleParser::RuleParser(ByteBuffer** buffer, size_t input_count):
    pass((Pass*)malloc(sizeof(Pass))), ruleBuffer(new RuleBuffer)
{
    pass->input = buffer;
    pass->input_count = input_count;
    pass->output = (Buffer**)&ruleBuffer;
    pass->output_count = 1;
    pass->contextMap = NULL;
    pass->run = run_RuleParser;
    pass->free = free_RuleParser;
}
Pass* & RuleParser::getPass(){
    return pass;
}
RuleBuffer* RuleParser::getRuleBuffer(){
    return ruleBuffer;
}