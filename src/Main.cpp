#include "Game.h"
#include <cstring>

#ifdef _WIN32

extern "C" {

__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#endif

bool HandleDebugCommand(int argc, const char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0) {
            return true;
        }
    }

    return false;
}

int main(int argc, const char* argv[])
{
    const bool isDebugMode = HandleDebugCommand(argc, argv);

    Game game;

    if (game.Initialize(isDebugMode)) {
        game.RunLoop();
        game.Shutdown();
    }

    return 0;
}