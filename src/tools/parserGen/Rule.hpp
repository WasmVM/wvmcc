#ifndef WVMCC_RULE_DEF
#define WVMCC_RULE_DEF

#include <string>
#include <vector>
#include <cstddef>

#define restrict __restrict__
#define _Bool bool
extern "C"{
    #include <adt/Buffer.h>
}
#undef restrict

class Rule{
public:
    class Element{
    public:
        std::string name;
        bool isOptional;
    };
    std::string target;
    std::vector<Element> elements;
    void print();
};

class RuleBuffer{
    Buffer buffer;
public:
    RuleBuffer();
    virtual ~RuleBuffer();
    Buffer* getBuffer();
    void addRule(Rule& rule);
    Rule& getRule(std::string target, size_t index);
};

#endif