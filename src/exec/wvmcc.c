#include <Option.h>

int main(int argc, const char *argv[])
{
    Option* option = new_Option(argc - 1, argv + 1);
    free_Option(&option);
    return 0;
}