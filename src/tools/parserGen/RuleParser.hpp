#ifndef WVMCC_RULEPARSER_DEF
#define WVMCC_RULEPARSER_DEF

#include <Rule.hpp>

#define restrict __restrict__
#define _Bool bool
extern "C"{
    #include <adt/Pass.h>
    #include <ByteBuffer.h>
}
#undef restrict

class RuleParser{
    Pass pass;
    RuleBuffer* ruleBuffer;
public:
    RuleParser(ByteBuffer* buffer);
    Pass* getPass();
    RuleBuffer* getRuleBuffer();
};

#endif
