#include <Option.h>

int main(int argc, const char *argv[])
{
    Option* option = new_Option(argc - 1, argv + 1);
    option->free(option);
    return 0;
}