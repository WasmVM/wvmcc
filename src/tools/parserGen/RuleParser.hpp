#ifndef WVMCC_RULEPARSER_DEF
#define WVMCC_RULEPARSER_DEF

#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <queue>
#include <iostream>
#include <parserGen/Rule.hpp>

#define restrict __restrict__
#define _Bool bool
extern "C"{
    #include <adt/Pass.h>
    #include <ByteBuffer.h>
}
#undef restrict

class RuleParser{
    Pass* pass;
    RuleBuffer* ruleBuffer;
public:
    RuleParser(ByteBuffer** buffer, size_t input_count);
    Pass* & getPass();
    RuleBuffer* getRuleBuffer();
};

#endif
