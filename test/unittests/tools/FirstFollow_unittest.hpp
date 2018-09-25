#include <skypat/skypat.h>

#include <parserGen/FirstFollow.hpp>

SKYPAT_F(FirstFollow_unittest, create)
{
    FirstFollow firstfollow;
    FirstFollowMap* firstMap = (FirstFollowMap*)firstfollow.getBuffer()->data;
    EXPECT_EQ(firstMap->size(), 0);
    EXPECT_EQ(firstfollow.getBuffer()->length, 0);
}

SKYPAT_F(FirstFollow_unittest, get_buffer)
{
    FirstFollow firstfollow;
    EXPECT_EQ(firstfollow.getBuffer()->length, 0);
    EXPECT_NE(firstfollow.getBuffer()->data, NULL);
}
SKYPAT_F(FirstFollow_unittest, begin)
{
    FirstFollow firstfollow;
    FirstFollowMap* firstMap = (FirstFollowMap*)firstfollow.getBuffer()->data;
    EXPECT_TRUE(firstfollow.begin() == firstMap->begin());
}

SKYPAT_F(FirstFollow_unittest, end)
{
    FirstFollow firstfollow;
    FirstFollowMap* firstMap = (FirstFollowMap*)firstfollow.getBuffer()->data;
    EXPECT_TRUE(firstfollow.end() == firstMap->end());
}

SKYPAT_F(FirstFollow_unittest, find)
{
    FirstFollow firstfollow;
    firstfollow.addElement("TestTarget", "TestElement");
    EXPECT_TRUE(firstfollow.find("TestTarget") != firstfollow.end());
    EXPECT_TRUE(firstfollow.find("Test") == firstfollow.end());
}
SKYPAT_F(FirstFollow_unittest, add_element)
{
    FirstFollow firstfollow;
    firstfollow.addElement("TestTarget", "TestElement");
    FirstFollowMap* firstMap = (FirstFollowMap*)firstfollow.getBuffer()->data;
    EXPECT_EQ((*firstMap)["TestTarget"].size(), 1);
    EXPECT_TRUE((*firstMap)["TestTarget"].front() == "TestElement");
}