#include "Game.h"

#include <cstring>
#include <filesystem>
#include <string>

#ifdef _WIN32

// Windows環境で高性能GPUを優先して使用する
// 切り替え可能GPU環境で内蔵GPUが選択されることによる性能低下を防ぐ
extern "C" {

__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#endif

struct GameLaunchOptions {
// 審査時に開発用エディタも確認できるよう、レビュー版ではデバッグツールを有効化する。
#ifdef GAME_REVIEW_BUILD
    bool areDebugToolsEnabled = true;
#else
    bool areDebugToolsEnabled = false;
#endif
    bool shouldStartInDebugStage = false;
    // Build & Restart後に直前のエディタ状態を復元するためのセッションファイル
    std::string editorSessionPath;
    // Build & Restart失敗時に再起動後のエディタへ渡すビルドログ
    std::string editorRestartErrorLogPath;
};

GameLaunchOptions ParseLaunchOptions(int argc, const char* argv[])
{
    GameLaunchOptions launchOptions;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0) {
            launchOptions.areDebugToolsEnabled = true;
            launchOptions.shouldStartInDebugStage = true;
            continue;
        }

        if (std::strcmp(argv[i], "--restore-editor-session") == 0 && i + 1 < argc) {
            launchOptions.editorSessionPath = argv[++i];
            launchOptions.areDebugToolsEnabled = true;
            launchOptions.shouldStartInDebugStage = true;
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
    }
}

int main(int argc, const char* argv[])
{
    UseExecutableDirectoryForPackagedBuild(argv[0]);
    const GameLaunchOptions launchOptions = ParseLaunchOptions(argc, argv);

    Game game;

    if (game.Initialize(
            launchOptions.areDebugToolsEnabled,
            launchOptions.shouldStartInDebugStage,
            launchOptions.editorSessionPath,
            launchOptions.editorRestartErrorLogPath)) {
        game.RunLoop();
        game.Shutdown();
    }

    return 0;
}
