#include "Game.h"
#include <cstring>
#include <filesystem>
#include <string>

#ifdef _WIN32

extern "C" {

__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#endif

struct GameLaunchOptions {
    bool isDebugMode = false;
    std::string editorSessionPath;
    std::string editorRestartErrorLogPath;
};

GameLaunchOptions ParseLaunchOptions(int argc, const char* argv[])
{
    GameLaunchOptions launchOptions;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0) {
            launchOptions.isDebugMode = true;
            continue;
        }

        if (std::strcmp(argv[i], "--restore-editor-session") == 0 && i + 1 < argc) {
            launchOptions.editorSessionPath = argv[++i];
            launchOptions.isDebugMode = true;
            continue;
        }

        if (std::strcmp(argv[i], "--editor-restart-error-log") == 0 && i + 1 < argc) {
            launchOptions.editorRestartErrorLogPath = argv[++i];
        }
    }

    return launchOptions;
}

void UseExecutableDirectoryForPackagedBuild(const char* executablePathText)
{
    const std::filesystem::path executablePath =
        std::filesystem::absolute(executablePathText);
    const std::filesystem::path executableDirectory =
        executablePath.parent_path();
    if (std::filesystem::is_directory(executableDirectory / "../assets") &&
        std::filesystem::is_directory(executableDirectory / "../shaders")) {
        std::filesystem::current_path(executableDirectory);
        return;
    }

    const std::filesystem::path currentDirectory =
        std::filesystem::current_path();
    if (std::filesystem::is_directory(currentDirectory / "../assets") &&
        std::filesystem::is_directory(currentDirectory / "../shaders")) {
        return;
    }
}

int main(int argc, const char* argv[])
{
    UseExecutableDirectoryForPackagedBuild(argv[0]);
    const GameLaunchOptions launchOptions = ParseLaunchOptions(argc, argv);

    Game game;

    if (game.Initialize(
            launchOptions.isDebugMode,
            launchOptions.editorSessionPath,
            launchOptions.editorRestartErrorLogPath)) {
        game.RunLoop();
        game.Shutdown();
    }

    return 0;
}
