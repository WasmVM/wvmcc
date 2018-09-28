#include <skypat/skypat.h>

#include <parserGen/FirstFollowExtractor.hpp>

#include <cstring>
#include <cstdlib>

#define restrict __restrict__
#define _Bool bool
extern "C"{
    #include <adt/Map.h>
}
#undef restrict

SKYPAT_F(FirstFollowExtractor_unittest, create_delete)
{
    // Setup
    RuleBuffer buffer;
    Rule rule("Test1");
    rule.elements.push_back("derive1");
    rule.elements.push_back("derive2");
    buffer.addRule(rule);
    
    // Test
    FirstFollowExtractor extractor(&buffer);
    EXPECT_EQ(extractor.getPass()->input_count, 1);
    EXPECT_EQ(extractor.getPass()->output_count, 2);
    EXPECT_EQ(extractor.getPass()->input[0], buffer.getBuffer());
    EXPECT_EQ(extractor.getPass()->output[0], (Buffer*)extractor.getFirstSet());
    EXPECT_EQ(extractor.getPass()->output[1], (Buffer*)extractor.getFollowSet());
}

SKYPAT_F(FirstFollowExtractor_unittest, first_set)
{
    // Setup
    RuleBuffer buffer;
    Rule* rules = new Rule[5]{
        Rule("Test1"),
        Rule("Test2"),
        Rule("Test1"),
        Rule("Test2"),
        Rule("Test2")
    };
    rules[0].elements.push_back("derive1");
    rules[0].elements.push_back("derive2");
    buffer.addRule(rules[0]);
    rules[1].elements.push_back("Test1");
    rules[1].elements.push_back("derive2");
    buffer.addRule(rules[1]);
    rules[2].elements.push_back("derive3");
    buffer.addRule(rules[2]);
    buffer.addRule(rules[3]);
    rules[4].elements.push_back("Test1");
    buffer.addRule(rules[4]);

    // Test
    FirstFollowExtractor extractor(&buffer);
    FirstFollow* firstSet = extractor.getFirstSet();
    Pass* pass = extractor.getPass();
    pass->contextMap = mapNew((keyComp_t)strcmp, free, free);
    const char* startTarget = "Test2";
    mapInsert(pass->contextMap, (void*)"start", (void*)startTarget);
    pass->run(pass);

    // Check first
    EXPECT_EQ(firstSet->find("Test1")->second.size(), 2);
    EXPECT_EQ(firstSet->find("Test2")->second.size(), 3);
    std::list<std::string>::iterator testIt = firstSet->find("Test1")->second.begin();
    EXPECT_EQ(*testIt++, "derive1");
    EXPECT_EQ(*testIt++, "derive3");
    testIt = firstSet->find("Test2")->second.begin();
    EXPECT_EQ(*testIt++, "derive1");
    EXPECT_EQ(*testIt++, "derive3");
    EXPECT_EQ(*testIt++, "");

    // Clean
    delete [] rules;
}

SKYPAT_F(FirstFollowExtractor_unittest, follow_set)
{
// Setup
    RuleBuffer buffer;
    Rule* rules = new Rule[5]{
        Rule("Test1"),
        Rule("Test2"),
        Rule("Test1"),
        Rule("Test2"),
        Rule("Test2")
    };
    rules[0].elements.push_back("derive1");
    rules[0].elements.push_back("derive2");
    buffer.addRule(rules[0]);
    rules[1].elements.push_back("Test1");
    rules[1].elements.push_back("derive2");
    buffer.addRule(rules[1]);
    rules[2].elements.push_back("derive3");
    buffer.addRule(rules[2]);
    buffer.addRule(rules[3]);
    rules[4].elements.push_back("Test1");
    buffer.addRule(rules[4]);

    // Test
    FirstFollowExtractor extractor(&buffer);
    FirstFollow* followSet = extractor.getFollowSet();
    Pass* pass = extractor.getPass();
    pass->contextMap = mapNew((keyComp_t)strcmp, free, free);
    const char* startTarget = "Test2";
    mapInsert(pass->contextMap, (void*)"start", (void*)startTarget);
    pass->run(pass);

    // Check
    EXPECT_EQ(followSet->find("Test1")->second.size(), 2);
    EXPECT_EQ(followSet->find("Test2")->second.size(), 1);
    std::list<std::string>::iterator testIt = followSet->find("Test1")->second.begin();
    EXPECT_EQ(*testIt++, "derive2");
    EXPECT_EQ(*testIt++, "$");
    testIt = followSet->find("Test2")->second.begin();
    EXPECT_EQ(*testIt++, "$");

    // Clean
    delete [] rules;
}