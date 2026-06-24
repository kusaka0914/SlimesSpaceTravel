#include "Game.h"
#include <cstring>

int main(int argc, const char* argv[])
{
    bool isDebugMode = false;
    if (argc > 1) {
        if (strcmp(argv[1], "--debug") == 0) {
            isDebugMode = true;
        }
    }
    Game game;
    bool success = game.Initialize(isDebugMode);
    if (success) {
        game.RunLoop();
        game.Shutdown();
    }
    return 0;
}