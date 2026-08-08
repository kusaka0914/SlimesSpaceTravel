#include "gfx/debug/assets/EditorAssetCatalog.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {
std::string ToLower(std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return text;
}

bool IsSupportedAssetExtension(
    EditorAssetType type,
    const std::filesystem::path& path)
{
    const std::string extension = ToLower(path.extension().string());

    switch (type) {
    case EditorAssetType::Model:
        return extension == ".obj" || extension == ".fbx" ||
               extension == ".gltf" || extension == ".glb" ||
               extension == ".dae";
    case EditorAssetType::Texture:
        return extension == ".png" || extension == ".jpg" ||
               extension == ".jpeg" || extension == ".bmp" ||
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
}

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

    std::sort(
        mAllAssets.begin(),
        mAllAssets.end(),
        [](const EditorAssetInfo& left, const EditorAssetInfo& right) {
            if (left.type != right.type) {
                return left.type < right.type;
            }
            return left.relativePath < right.relativePath;
        });

    mScanStatus =
        "モデル " + std::to_string(GetPaths(EditorAssetType::Model).size()) +
        " / 画像 " + std::to_string(GetPaths(EditorAssetType::Texture).size()) +
        " / 動画 " + std::to_string(GetPaths(EditorAssetType::Video).size());
    mHasScanned = true;
}

const std::vector<EditorAssetInfo>& EditorAssetCatalog::GetAllAssets() const
{
    return mAllAssets;
}

const std::vector<std::string>& EditorAssetCatalog::GetPaths(
    EditorAssetType type) const
{
    return mPathsByType[ToIndex(type)];
}

const std::string& EditorAssetCatalog::GetScanStatus() const
{
    return mScanStatus;
}

void EditorAssetCatalog::ScanAssetType(
    EditorAssetType type,
    const std::string& directoryName)
{
    const std::filesystem::path assetsRoot("../assets");
    const std::filesystem::path typeRoot = assetsRoot / directoryName;
    std::error_code error;
    if (!std::filesystem::is_directory(typeRoot, error)) {
        return;
    }

    std::vector<std::string>& typePaths = mPathsByType[ToIndex(type)];
    for (std::filesystem::recursive_directory_iterator iterator(typeRoot, error), end;
         iterator != end && !error;
         iterator.increment(error)) {
        if (!iterator->is_regular_file(error) ||
            !IsSupportedAssetExtension(type, iterator->path())) {
            continue;
        }

        const std::filesystem::path relativeBase =
            type == EditorAssetType::Model ? typeRoot : assetsRoot;
        const std::filesystem::path relativePath =
            std::filesystem::relative(iterator->path(), relativeBase, error);
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
