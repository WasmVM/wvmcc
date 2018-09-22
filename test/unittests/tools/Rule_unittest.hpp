#include <skypat/skypat.h>

#include <parserGen/Rule.hpp>

#include <sstream>

SKYPAT_F(Rule_unittest, Rule_addElement)
{
    Rule rule;
    rule.target = "Test";
    rule.elements.push_back("derive1");
    rule.elements.push_back("derive2");
    EXPECT_EQ(rule.elements.at(0), "derive1");
    EXPECT_EQ(rule.elements.at(1), "derive2");
}

SKYPAT_F(Rule_unittest, Rule_print)
{
    Rule rule;
    rule.target = "Test";
    rule.elements.push_back("derive1");
    rule.elements.push_back("derive2");

    std::streambuf *originalBuf = std::cout.rdbuf();
    std::ostringstream ostream;
    std::cout.rdbuf(ostream.rdbuf());
    rule.print();
    std::cout.rdbuf(originalBuf);
    EXPECT_EQ(ostream.str(), "Test => derive1 derive2 \n");
}

SKYPAT_F(Rule_unittest, RuleBuffer_get_buffer)
{
    RuleBuffer ruleBuffer;
    Buffer* buffer = ruleBuffer.getBuffer();
    EXPECT_EQ(buffer->length, 0);
    EXPECT_NE(buffer->data, NULL);
}

SKYPAT_F(Rule_unittest, RuleBuffer_begin)
{
    RuleBuffer ruleBuffer;
    RuleMap& ruleMap = *(RuleMap*)ruleBuffer.getBuffer()->data;
    EXPECT_TRUE(ruleBuffer.begin() == ruleMap.begin());
}

SKYPAT_F(Rule_unittest, RuleBuffer_end)
{
    RuleBuffer ruleBuffer;
    RuleMap& ruleMap = *(RuleMap*)ruleBuffer.getBuffer()->data;
    EXPECT_TRUE(ruleBuffer.end() == ruleMap.end());
}

SKYPAT_F(Rule_unittest, RuleBuffer_find)
{
    RuleBuffer ruleBuffer;
    Rule rule;
    rule.target = "TestTarget";
    rule.elements.push_back("derive");
    ruleBuffer.addRule(rule);
    EXPECT_TRUE(ruleBuffer.find("TestTarget") != ruleBuffer.end());
    EXPECT_TRUE(ruleBuffer.find("Test") == ruleBuffer.end());
}

SKYPAT_F(Rule_unittest, RuleBuffer_add_rule)
{
    RuleBuffer ruleBuffer;
    Rule rule;
    rule.target = "TestTarget";
    rule.elements.push_back("derive");
    ruleBuffer.addRule(rule);

    RuleMap& ruleMap = *(RuleMap*)ruleBuffer.getBuffer()->data;
    EXPECT_TRUE(ruleMap["TestTarget"].at(0).target == "TestTarget");
    EXPECT_TRUE(ruleMap["TestTarget"].at(0).elements.at(0) == "derive");
}

SKYPAT_F(Rule_unittest, RuleBuffer_get_rule)
{
    RuleBuffer ruleBuffer;
    Rule rule;
    rule.target = "TestTarget";
    rule.elements.push_back("derive");
    ruleBuffer.addRule(rule);

    Rule& result = ruleBuffer.getRule("TestTarget", 0);
    EXPECT_TRUE(result.target == "TestTarget");
    EXPECT_TRUE(result.elements.at(0) == "derive");
}

SKYPAT_F(Rule_unittest, RuleBuffer_get_target_rules)
{
    RuleBuffer ruleBuffer;
    Rule rule;
    rule.target = "TestTarget";
    rule.elements.push_back("derive");
    ruleBuffer.addRule(rule);

    RuleMap::iterator result = ruleBuffer.getTargetRules("TestTarget");
    EXPECT_TRUE(result->second.at(0).target == "TestTarget");
    EXPECT_TRUE(result->second.at(0).elements.at(0) == "derive");
}