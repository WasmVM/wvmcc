#ifndef WVMCC_RULE_DEF
#define WVMCC_RULE_DEF

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <cstddef>
#include <cstdlib>

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
        Element(std::string name = "", bool isOptional = false);
        std::string name;
        bool isOptional;
    };
    std::string target;
    std::vector<Element> elements;
    Rule(std::string target = "");
    void addElement(std::string name = "", bool isOptional = false);
    void print();
};

typedef std::map<std::string, std::vector<Rule>> RuleMap;

class RuleBuffer{
    Buffer buffer;
public:
    RuleBuffer();
    virtual ~RuleBuffer();
    Buffer* getBuffer();
    RuleMap::iterator begin();
    RuleMap::iterator end();
    RuleMap::iterator find(const std::string target);
    void addRule(const Rule& rule);
    Rule& getRule(const std::string target, const size_t index);
    RuleMap::iterator getTargetRules(const std::string target);
};

#endif