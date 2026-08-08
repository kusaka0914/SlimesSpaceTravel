#pragma once

#include <array>
#include <string>
#include <vector>

enum class EditorAssetType {
    Model = 0,
    Texture,
    Video,
    Count,
};

struct EditorAssetInfo {
    EditorAssetType type = EditorAssetType::Model;
    std::string relativePath;
};

class EditorAssetCatalog {
public:
    void EnsureScanned();
    void Refresh();

    const std::vector<EditorAssetInfo>& GetAllAssets() const;
    const std::vector<std::string>& GetPaths(EditorAssetType type) const;
    const std::string& GetScanStatus() const;

private:
    void ScanAssetType(
        EditorAssetType type,
        const std::string& directoryName);

    std::array<std::vector<std::string>, static_cast<std::size_t>(EditorAssetType::Count)>
        mPathsByType;
    std::vector<EditorAssetInfo> mAllAssets;
    std::string mScanStatus;
    bool mHasScanned = false;
};
