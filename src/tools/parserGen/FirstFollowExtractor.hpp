#ifndef WVMCC_PARSERGEN_FIRSTFOLLOWEXTRACTOR_DEF
#define WVMCC_PARSERGEN_FIRSTFOLLOWEXTRACTOR_DEF

#include <cstdlib>

#include <parserGen/Rule.hpp>
#include <parserGen/FirstFollow.hpp>

#define restrict __restrict__
#define _Bool bool
extern "C"{
    #include <adt/Pass.h>
    #include <adt/Map.h>
}
#undef restrict

#define endFlag "$"

class FirstFollowExtractor{
    Pass* pass;
    FirstFollow *firstSet;
    FirstFollow *followSet;
    
public:
    FirstFollowExtractor(RuleBuffer& buffer);
    Pass* & getPass();
    FirstFollow* & getFirstSet();
    FirstFollow* & getFollowSet();
};

#endif