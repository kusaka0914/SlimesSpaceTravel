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

    // Modelはmodels相対、Texture・Videoはassets相対のパス。区切りは/。
    std::string relativePath;
};

// assets内の対応形式を走査し、種類別と全件のパス一覧を保持する。
class EditorAssetCatalog {
public:
    // 未走査の場合だけRefreshを行う。
    void EnsureScanned();

    // 一覧を作り直し、種類・パス順に並べる。存在しないフォルダーは空として扱い、走査エラー時は取得済み分を残す。
    void Refresh();

    const std::vector<EditorAssetInfo>& GetAllAssets() const;

    // typeにはModel・Texture・Videoを渡す。走査は行わず、保持中の一覧を返す。
    const std::vector<std::string>& GetPaths(EditorAssetType type) const;

    const std::string& GetScanStatus() const;

private:
    void ScanAssetType(EditorAssetType type, const std::string& directoryName);

    std::array<std::vector<std::string>, static_cast<std::size_t>(EditorAssetType::Count)> mPathsByType;
    std::vector<EditorAssetInfo> mAllAssets;
    std::string mScanStatus;
    bool mHasScanned = false;
};
