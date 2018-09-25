#ifndef WVMCC_PARSERGEN_FIRSTFOLLOW_DEF
#define WVMCC_PARSERGEN_FIRSTFOLLOW_DEF

#include <cstdlib>
#include <map>
#include <string>
#include <list>

#define restrict __restrict__
#define _Bool bool
extern "C"{
    #include <adt/Buffer.h>
}
#undef restrict

typedef std::map<std::string, std::list<std::string>> FirstFollowMap;

class FirstFollow{
    Buffer buffer;
public:
    FirstFollow();
    virtual ~FirstFollow();
    Buffer* getBuffer();
    FirstFollowMap::iterator begin();
    FirstFollowMap::iterator end();
    FirstFollowMap::iterator find(const std::string target);
    void addElement(const std::string target, const std::string element);
};

#endif