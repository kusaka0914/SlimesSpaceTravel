#include "app/Game.h"

#include <cstring>
#include <filesystem>
#include <string>

#ifdef _WIN32

// 切り替え可能なGPU環境で高性能GPUを要求するため、ドライバーが参照するシンボルを公開する。
extern "C" {

__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#endif

namespace {

struct GameLaunchOptions {
    // レビュー版はタイトルから起動しつつ、開発用エディターを利用可能にする。
#ifdef GAME_REVIEW_BUILD
    bool areDebugToolsEnabled = true;
#else
    bool areDebugToolsEnabled = false;
#endif
    bool shouldStartInDebugStage = false;

    // Build & Restart後に直前のエディター状態を復元するためのセッションファイル。
    std::string editorSessionPath;

    // Build & Restart失敗時に再起動後のエディターへ渡すビルドログ。
    std::string editorRestartErrorLogPath;
};

GameLaunchOptions ParseLaunchOptions(int argc, const char* argv[])
{
    GameLaunchOptions launchOptions;
    for (int argumentIndex = 1; argumentIndex < argc; ++argumentIndex) {
        if (std::strcmp(argv[argumentIndex], "--debug") == 0) {
            launchOptions.areDebugToolsEnabled = true;
            launchOptions.shouldStartInDebugStage = true;
            continue;
        }

        if (std::strcmp(argv[argumentIndex], "--restore-editor-session") == 0 && argumentIndex + 1 < argc) {
            launchOptions.editorSessionPath = argv[++argumentIndex];
            launchOptions.areDebugToolsEnabled = true;
            launchOptions.shouldStartInDebugStage = true;
            continue;
        }

        if (std::strcmp(argv[argumentIndex], "--editor-restart-error-log") == 0 && argumentIndex + 1 < argc) {
            launchOptions.editorRestartErrorLogPath = argv[++argumentIndex];
        }
    }

    return launchOptions;
}

void UseExecutableDirectoryForPackagedBuild(const char* executablePathText)
{
    const std::filesystem::path executablePath = std::filesystem::absolute(executablePathText);
    const std::filesystem::path executableDirectory = executablePath.parent_path();

    // 配布構成が揃う場合だけ基準ディレクトリを変え、相対パスのassets・shadersを解決できるようにする。
    if (std::filesystem::is_directory(executableDirectory / "../assets") &&
        std::filesystem::is_directory(executableDirectory / "../shaders")) {
        std::filesystem::current_path(executableDirectory);
    }
}

} // namespace

int main(int argc, const char* argv[])
{
    UseExecutableDirectoryForPackagedBuild(argv[0]);
    const GameLaunchOptions launchOptions = ParseLaunchOptions(argc, argv);

    Game game;

    if (game.Initialize(launchOptions.areDebugToolsEnabled, launchOptions.shouldStartInDebugStage,
                        launchOptions.editorSessionPath, launchOptions.editorRestartErrorLogPath)) {
        game.RunLoop();
        game.Shutdown();
    }

    return 0;
}
