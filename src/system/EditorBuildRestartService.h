#pragma once

#include <filesystem>
#include <string>

class EditorBuildRestartService {
public:
    bool ResolveSessionFilePath(
        std::filesystem::path& outSessionFilePath,
        std::string& outErrorMessage) const;

    bool ResolvePersistentDebugSessionFilePath(
        std::filesystem::path& outSessionFilePath,
        std::string& outErrorMessage) const;

    bool LaunchBuildAndRestartHelper(
        const std::filesystem::path& sessionFilePath,
        std::string& outErrorMessage) const;

private:
    struct RuntimePaths {
        std::filesystem::path gameExecutable;
        std::filesystem::path helperExecutable;
        std::filesystem::path buildDirectory;
        std::filesystem::path buildLog;
        std::string configuration;
    };

    bool ResolveRuntimePaths(
        RuntimePaths& outRuntimePaths,
        std::string& outErrorMessage) const;
};
