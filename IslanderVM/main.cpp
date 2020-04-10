#include <iostream>
#include "vm.h"
#include "parser.h"

int main(int count, const char** args)
{
    if (count >= 2)
    {
        vm_options options;
        options.x64 = false;

        for (int i = 0; i < count; i++)
        {
            if (strcmp("--x64", args[i]) == 0)
            {
                options.x64 = true;
            }
        }

        const int errorLength = 256;
        char errorText[errorLength];
        if (!load_file_and_execute(args[1], options, errorText, errorLength))
        {
            std::cout << errorText << std::endl;
        }
    }
    else
    {
        std::cout << "No filename given" << std::endl;
    }

    return 0;
}

