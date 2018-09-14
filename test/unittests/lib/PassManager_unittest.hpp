#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <PassManager.h>
    #include <string.h>
}
#undef restrict

SKYPAT_F(PassManager_unittest, create_delete)
{
    PassManager* passManager = new_PassManager();
    EXPECT_EQ(passManager->passList->size, 0);
    free_PassManager(&passManager);
    EXPECT_EQ(passManager, NULL);
}