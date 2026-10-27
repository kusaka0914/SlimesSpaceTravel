#include "editor/assets/EditorAssetCatalog.h"

#include "utils/StringUtils.h"

#include <algorithm>
#include <filesystem>

namespace {

bool IsSupportedAssetExtension(EditorAssetType type, const std::filesystem::path& path)
{
    const std::string extension = StringUtils::ToLower(path.extension().string());

    switch (type) {
    case EditorAssetType::Model:
        // 衝突専用モデルは描画・配置用のアセット一覧へ出さない。
        if (StringUtils::ToLower(path.filename().string()).ends_with(".collision.obj")) {
            return false;
        }
        return extension == ".obj" || extension == ".fbx" || extension == ".gltf" || extension == ".glb" ||
               extension == ".dae";
    case EditorAssetType::Texture:
        return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" ||
               extension == ".tga";
    case EditorAssetType::Video:
        return extension == ".mp4";
    case EditorAssetType::Count:
        return false;
    }

    return false;
}

std::size_t ToIndex(EditorAssetType type)
{
    return static_cast<std::size_t>(type);
}
} // namespace

void EditorAssetCatalog::EnsureScanned()
{
    if (!mHasScanned) {
        Refresh();
    }
}

void EditorAssetCatalog::Refresh()
{
    for (std::vector<std::string>& paths : mPathsByType) {
        paths.clear();
    }
    mAllAssets.clear();

    ScanAssetType(EditorAssetType::Model, "models");
    ScanAssetType(EditorAssetType::Texture, "textures");
    ScanAssetType(EditorAssetType::Video, "videos");

    std::sort(mAllAssets.begin(), mAllAssets.end(), [](const EditorAssetInfo& left, const EditorAssetInfo& right) {
        if (left.type != right.type) {
            return left.type < right.type;
        }
        return left.relativePath < right.relativePath;
    });

    mScanStatus = "モデル " + std::to_string(GetPaths(EditorAssetType::Model).size()) + " / 画像 " +
                  std::to_string(GetPaths(EditorAssetType::Texture).size()) + " / 動画 " +
                  std::to_string(GetPaths(EditorAssetType::Video).size());
    mHasScanned = true;
}

const std::vector<EditorAssetInfo>& EditorAssetCatalog::GetAllAssets() const
{
    return mAllAssets;
}

const std::vector<std::string>& EditorAssetCatalog::GetPaths(EditorAssetType type) const
{
    return mPathsByType[ToIndex(type)];
}

const std::string& EditorAssetCatalog::GetScanStatus() const
{
    return mScanStatus;
}

void EditorAssetCatalog::ScanAssetType(EditorAssetType type, const std::string& directoryName)
{
    const std::filesystem::path assetsRoot("../assets");
    const std::filesystem::path typeRoot = assetsRoot / directoryName;
    std::error_code error;
    if (!std::filesystem::is_directory(typeRoot, error)) {
        return;
    }

    std::vector<std::string>& typePaths = mPathsByType[ToIndex(type)];
    for (std::filesystem::recursive_directory_iterator iterator(typeRoot, error), end; iterator != end && !error;
         iterator.increment(error)) {
        if (!iterator->is_regular_file(error) || !IsSupportedAssetExtension(type, iterator->path())) {
            continue;
        }

        // モデル読込はmodels基準、画像・動画の読込はassets基準のパスを受け取る。
        const std::filesystem::path relativeBase = type == EditorAssetType::Model ? typeRoot : assetsRoot;
        const std::filesystem::path relativePath = std::filesystem::relative(iterator->path(), relativeBase, error);
        if (error) {
            error.clear();
            continue;
        }

        const std::string normalizedPath = relativePath.generic_string();
        typePaths.emplace_back(normalizedPath);
        mAllAssets.push_back({type, normalizedPath});
    }

    std::sort(typePaths.begin(), typePaths.end());
}
