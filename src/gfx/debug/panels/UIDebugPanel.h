#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/ui/UICanvasEditorController.h"
#include "system/UILoadSystem.h"

#include <array>
#include <string>
#include <vector>

class UIDebugPanel : public DebugPanel {
public:
    explicit UIDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    enum class SelectedElementSource {
        None,
        Custom,
        ExistingTexture,
        ExistingText,
    };

    void DrawUIEditor(UILoadSystem* uiLoadSystem);
    void DrawCanvasToolbar();
    void DrawElementList(UILoadSystem* uiLoadSystem);
    void DrawElementInspector(UILoadSystem* uiLoadSystem);
    void DrawCustomElementInspector(UILoadSystem* uiLoadSystem);
    void DrawExistingTextureInspector(UILoadSystem* uiLoadSystem);
    void DrawExistingTextInspector(UILoadSystem* uiLoadSystem);
    void DrawCodeBoundElementProtection();
    void DrawAssetPicker(
        std::string& texturePath,
        bool& flipVertical,
        const char* widgetId,
        const char* sectionLabel);
    void SaveAllUI(UILoadSystem* uiLoadSystem);
    void ReloadAllUI(UILoadSystem* uiLoadSystem);

    std::string GetDisplayName(const std::string& key) const;

private:
    UICanvasEditorController mCanvasEditor;
    int mNewElementType = 0;
    std::array<char, 128> mNewScreen = {"custom"};
    std::array<char, 128> mNewId = {"element"};
    std::array<char, 128> mAssetFilter = {};
    int mSelectedInputDeviceImageVariant = 0;
    SelectedElementSource mSelectedElementSource = SelectedElementSource::None;
    std::string mSelectedExistingElementKey;
    std::string mStatusMessage;
};
