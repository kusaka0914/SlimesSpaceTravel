#pragma once

#include "gfx/debug/DebugPanel.h"
#include "system/UILoadSystem.h"

#include <array>
#include <string>
#include <vector>

class UIDebugPanel : public DebugPanel {
public:
    explicit UIDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawTextures(UILoadSystem* uiLoadSystem);
    void DrawTexts(UILoadSystem* uiLoadSystem);
    void DrawCustomUIEditor(UILoadSystem* uiLoadSystem);
    void DrawCustomElementList(UILoadSystem* uiLoadSystem);
    void DrawCustomElementInspector(UILoadSystem* uiLoadSystem);
    void DrawAssetPicker(UILoadSystem::CustomElement& element);
    void RefreshTextureAssets();

    std::string GetDisplayName(const std::string& key) const;

private:
    int mSelectedCustomElement = -1;
    int mNewElementType = 0;
    std::array<char, 128> mNewScreen = {"custom"};
    std::array<char, 128> mNewId = {"element"};
    std::array<char, 128> mAssetFilter = {};
    std::vector<std::string> mTextureAssets;
    std::string mStatusMessage;
    bool mTextureAssetsScanned = false;
};
