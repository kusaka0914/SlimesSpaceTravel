#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"

#include <array>
#include <string>

class AssetBrowserPanel final : public DebugPanel {
public:
    explicit AssetBrowserPanel(DebugEditorContext& context);

    void Draw() override;

private:
    bool ShouldShow(const EditorAssetInfo& asset) const;
    const char* ResolveTypeLabel(EditorAssetType type) const;
    const char* ResolveDragDropPayload(EditorAssetType type) const;

    std::array<char, 128> mSearchText = {};
    std::string mSelectedAssetPath;
    int mSelectedType = -1;
};
