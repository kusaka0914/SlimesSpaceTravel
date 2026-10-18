#pragma once

#include <filesystem>
#include <string>

// 実行ファイルの配置からセッション保存先を解決し、再起動用ヘルパーを起動する。
class EditorBuildRestartService {
public:
    // 再起動用セッションの保存先を返す。失敗時はfalseとoutErrorMessageで理由を返す。
    bool ResolveSessionFilePath(std::filesystem::path& outSessionFilePath, std::string& outErrorMessage) const;

    // 通常起動で復元するセッションの保存先を返す。失敗時はfalseとoutErrorMessageで理由を返す。
    bool ResolvePersistentDebugSessionFilePath(std::filesystem::path& outSessionFilePath,
                                               std::string& outErrorMessage) const;

    // 親プロセスの終了を待つヘルパーを別プロセスで起動する。戻り値はビルド完了を表さない。
    // 起動要求に失敗した場合はfalseを返し、outErrorMessageに理由を設定する。
    bool LaunchBuildAndRestartHelper(const std::filesystem::path& sessionFilePath, std::string& outErrorMessage) const;

private:
    struct RuntimePaths {
        std::filesystem::path gameExecutable;
        std::filesystem::path helperExecutable;
        std::filesystem::path buildDirectory;
        std::filesystem::path buildLog;
        std::string configuration;
    };

    bool ResolveRuntimePaths(RuntimePaths& outRuntimePaths, std::string& outErrorMessage) const;
};
