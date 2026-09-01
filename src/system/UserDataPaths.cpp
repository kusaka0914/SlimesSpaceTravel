#include "system/UserDataPaths.h"

#include <cstdlib>
#include <memory>
#include <system_error>

namespace {

constexpr const char* ApplicationDirectoryName = "SlimesSpaceTravel";

std::filesystem::path FindPlatformDataDirectory()
{
#ifdef _WIN32
    wchar_t* appDataDirectoryText = nullptr;
    std::size_t appDataDirectoryLength = 0;
    const errno_t environmentReadResult = _wdupenv_s(
        &appDataDirectoryText,
        &appDataDirectoryLength,
        L"APPDATA");
    const std::unique_ptr<wchar_t, decltype(&std::free)>
        appDataDirectory(appDataDirectoryText, &std::free);
    if (environmentReadResult == 0 && appDataDirectory) {
        return std::filesystem::path(appDataDirectory.get());
    }
#elif defined(__APPLE__)
    if (const char* homeDirectory = std::getenv("HOME")) {
        return std::filesystem::path(homeDirectory) /
            "Library/Application Support";
    }
#else
    if (const char* dataDirectory = std::getenv("XDG_DATA_HOME")) {
        return std::filesystem::path(dataDirectory);
    }
    if (const char* homeDirectory = std::getenv("HOME")) {
        return std::filesystem::path(homeDirectory) / ".local/share";
    }
#endif

    return std::filesystem::current_path() / "user_data";
}

bool CopyFileIfMissing(
    const std::filesystem::path& sourceFile,
    const std::filesystem::path& destinationFile,
    std::error_code& fileSystemError)
{
    if (std::filesystem::exists(destinationFile, fileSystemError)) {
        return !fileSystemError;
    }
    if (fileSystemError) {
        return false;
    }

    if (!std::filesystem::exists(sourceFile, fileSystemError)) {
        return !fileSystemError;
    }
    if (fileSystemError) {
        return false;
    }

    std::filesystem::copy_file(
        sourceFile,
        destinationFile,
        std::filesystem::copy_options::none,
        fileSystemError);
    return !fileSystemError;
}

bool CopyYamlFilesIfMissing(
    const std::filesystem::path& sourceDirectory,
    const std::filesystem::path& destinationDirectory,
    std::error_code& fileSystemError)
{
    if (!std::filesystem::exists(sourceDirectory, fileSystemError)) {
        return !fileSystemError;
    }
    if (fileSystemError) {
        return false;
    }

    for (const std::filesystem::directory_entry& sourceEntry :
         std::filesystem::directory_iterator(
             sourceDirectory,
             fileSystemError)) {
        if (fileSystemError) {
            return false;
        }
        if (!sourceEntry.is_regular_file() ||
            sourceEntry.path().extension() != ".yaml") {
            continue;
        }

        const std::filesystem::path destinationFile =
            destinationDirectory / sourceEntry.path().filename();
        if (!CopyFileIfMissing(
                sourceEntry.path(),
                destinationFile,
                fileSystemError)) {
            return false;
        }
    }
    return true;
}

}

namespace UserDataPaths {

std::filesystem::path FindRootDirectory()
{
    return FindPlatformDataDirectory() / ApplicationDirectoryName;
}

std::filesystem::path ResolveStageProgressFile()
{
    return FindRootDirectory() / "stage_progress.yaml";
}

std::filesystem::path ResolveUGCWorkingStageFile()
{
    return FindRootDirectory() / "ugc_stage.yaml";
}

std::filesystem::path ResolveUGCSavedWorkDirectory()
{
    return FindRootDirectory() / "ugc_saves";
}

std::filesystem::path ResolveUGCTutorialStageFile()
{
    return FindRootDirectory() / "ugc_tutorial_stage.yaml";
}

bool PrepareFromPackagedAssets(
    const std::filesystem::path& assetDataDirectory,
    std::string& outErrorMessage)
{
    outErrorMessage.clear();
    std::error_code fileSystemError;
    const std::filesystem::path savedWorkDirectory =
        ResolveUGCSavedWorkDirectory();
    std::filesystem::create_directories(
        savedWorkDirectory,
        fileSystemError);
    if (fileSystemError) {
        outErrorMessage =
            "Failed to create user data directory: " +
            fileSystemError.message();
        return false;
    }

    const std::filesystem::path packagedStageDirectory =
        assetDataDirectory / "stage";
    if (!CopyFileIfMissing(
            assetDataDirectory / "save/stage_progress.yaml",
            ResolveStageProgressFile(),
            fileSystemError) ||
        !CopyYamlFilesIfMissing(
            packagedStageDirectory / "ugc_saves",
            savedWorkDirectory,
            fileSystemError)) {
        outErrorMessage =
            "Failed to migrate existing user data: " +
            fileSystemError.message();
        return false;
    }

    const std::filesystem::path packagedWorkingStage =
        packagedStageDirectory / "ugc_stage.yaml";
    const std::filesystem::path packagedStageTemplate =
        packagedStageDirectory / "ugc_stage_template.yaml";
    if (!std::filesystem::exists(packagedWorkingStage) &&
        !std::filesystem::exists(packagedStageTemplate)) {
        outErrorMessage = "The packaged UGC stage template is missing";
        return false;
    }
    const std::filesystem::path initialWorkingStage =
        std::filesystem::exists(packagedWorkingStage)
            ? packagedWorkingStage
            : packagedStageTemplate;
    if (!CopyFileIfMissing(
            initialWorkingStage,
            ResolveUGCWorkingStageFile(),
            fileSystemError)) {
        outErrorMessage =
            "Failed to prepare the UGC working stage: " +
            fileSystemError.message();
        return false;
    }

    return true;
}

}
