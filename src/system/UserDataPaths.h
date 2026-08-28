#pragma once

#include <filesystem>
#include <string>

namespace UserDataPaths {

std::filesystem::path FindRootDirectory();
std::filesystem::path ResolveStageProgressFile();
std::filesystem::path ResolveUGCWorkingStageFile();
std::filesystem::path ResolveUGCSavedWorkDirectory();
std::filesystem::path ResolveUGCTutorialStageFile();

bool PrepareFromPackagedAssets(
    const std::filesystem::path& assetDataDirectory,
    std::string& outErrorMessage);

}
