#include <cstdio>

#include "Application/Application.h"

int main(int Argc, char** Argv)
{
    Application App;
    if (!App.Initialise())
    {
        std::fprintf(stderr, "Failed to initialise application\n");
        return 1;
    }

    if (Argc > 1)
        App.LoadFile(Argv[1]);

    App.Run();
    return 0;
}
