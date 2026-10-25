#include "system/UserDataPaths.h"

#include <chrono>

// 統合テストだけでリンクする保存先。OSの保存先解決・既存セーブ移行はこのテストの対象外。
namespace UserDataPaths {

std::filesystem::path FindRootDirectory()
{
    static const auto root =
        std::filesystem::temp_directory_path() /
        ("slimes_runtime_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    return root;
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
std::filesystem::path ResolveTgsPlayLogDirectory()
{
    return FindRootDirectory() / "tgs_playlogs";
}

bool PrepareFromPackagedAssets(const std::filesystem::path& assets, std::string& outError)
{
    outError.clear();
    std::error_code error;
    std::filesystem::create_directories(ResolveUGCSavedWorkDirectory(), error);
    if (!error) {
        std::filesystem::copy_file(assets / "stage/ugc_stage_template.yaml", ResolveUGCWorkingStageFile(),
                                   std::filesystem::copy_options::skip_existing, error);
    }
    if (error) {
        outError = error.message();
    }
    return !error;
}

} // namespace UserDataPaths
