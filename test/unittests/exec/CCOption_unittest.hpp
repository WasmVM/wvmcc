#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <CCOption.h>
    #include <string.h>
}
#undef restrict

SKYPAT_F(CCOption_unittest, configure_output_file)
{
    int argc = 2;
    const char** argv = new const char* [2]{
        "-o",
        "test"
    };
    CCOption* option = new_CCOption(argc, argv);
    EXPECT_FALSE(strcmp(option->output_file, "test"));
    free_CCOption(&option);
}

SKYPAT_F(CCOption_unittest, configure_input_file)
{
    int argc = 3;
    const char* argv[3] = {
        "test.c",
        "test1.c",
        "test2.c"
    };
    CCOption* option = new_CCOption(argc, argv);
    EXPECT_FALSE(strcmp(option->input_files[0], "test.c"));
    EXPECT_FALSE(strcmp(option->input_files[1], "test1.c"));
    EXPECT_FALSE(strcmp(option->input_files[2], "test2.c"));
    free_CCOption(&option);
}